/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include <QMailStore>
#include <qmailnamespace.h>
#include <glib.h>
#include <libedataserver/e-account-list.h>
#include <libedataserver/e-list.h>
#include <gconf/gconf-client.h>
#include "emailaccountlistmodel.h"
#include "dbustypes.h"

void EmailAccountListModel::onGetPassword (const QString &title, const QString &prompt, const QString &key)
{
	qDebug() << "Ask Password\n\n";
	emit askPassword (title, prompt, key);
}

void EmailAccountListModel::onSendReceiveComplete()
{
	qDebug() << "Send Receive complete \n\n";
	emit sendReceiveCompleted();
}

EmailAccountListModel::EmailAccountListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    qDebug() << "EmailAccountListModel constructor";
    QHash<int, QByteArray> roles;

    registerMyDataTypes ();

    roles.insert(DisplayName, "displayName");
    roles.insert(EmailAddress, "emailAddress");
    roles.insert(MailServer, "mailServer");
    roles.insert(UnreadCount, "unreadCount");
    roles.insert(MailAccountId, "mailAccountId");
    setRoleNames(roles);

    GConfClient *client;

    client = gconf_client_get_default ();
    account_list = e_account_list_new (client);
    g_object_unref (client);

    /* Init here for password prompt */
    session_instance = OrgGnomeEvolutionDataserverMailSessionInterface::instance(this);
    QObject::connect (session_instance, SIGNAL(GetPassword(const QString &, const QString &, const QString &)), this, SLOT(onGetPassword (const QString &, const QString &, const QString &)));
    QObject::connect (session_instance, SIGNAL(sendReceiveComplete()), this, SLOT(onSendReceiveComplete()));
/*
    connect (QMailStore::instance(), SIGNAL(accountsAdded(const QMailAccountIdList &)), this,
             SLOT(onAccountsAdded (const QMailAccountIdList &)));
    connect (QMailStore::instance(), SIGNAL(accountsRemoved(const QMailAccountIdList &)), this,
             SLOT(onAccountsRemoved(const QMailAccountIdList &)));
    connect (QMailStore::instance(), SIGNAL(accountsUpdated(const QMailAccountIdList &)), this,
             SLOT(onAccountsUpdated(const QMailAccountIdList &)));

    QMailAccountListModel::setSynchronizeEnabled(true);
    QMailAccountListModel::setKey(QMailAccountKey::messageType(QMailMessage::Email));
*/
}

EmailAccountListModel::~EmailAccountListModel()
{
}

EAccount * EmailAccountListModel::getAccountByIndex(int idx) const
{
    EIterator *iter;
    int i=-1;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (account_list));
    for (; e_iterator_is_valid (iter); e_iterator_next (iter)) {
	EAccountService *service;

	account = (EAccount *) e_iterator_get (iter);
	service = account->source;
	
	/* Iterate over valid accounts only */
	if (service->url && *service->url) {
	        i++;
	}
	
	if (i == idx)
		break;
    }

    if (!e_iterator_is_valid (iter))
	account = NULL;
	
    g_object_unref (iter);

    return account;
}

EAccount * EmailAccountListModel::getAccountById(char *id)
{
    EIterator *iter;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
        account = (EAccount *) e_iterator_get (iter);
	if (strcmp (id, account->uid) == 0)
	     return account;
        e_iterator_next (iter);
    }

    g_object_unref (iter);

    return NULL;
}

int EmailAccountListModel::getIndexById(char *id)
{
    EIterator *iter;
    EAccount *account = NULL;
    int index=-1;

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
	EAccountService *service;

	account = (EAccount *) e_iterator_get (iter);
	service = account->source;
	
	/* Iterate over valid accounts only */
	if (service->url && *service->url)
        	index++;
        account = (EAccount *) e_iterator_get (iter);
        if (strcmp (id, account->uid) == 0)
             return index;
        e_iterator_next (iter);
    }

    g_object_unref (iter);

    return -1;
}


int EmailAccountListModel::rowCount(const QModelIndex &parent) const
{
    EIterator *iter;
    int i=0;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
	EAccountService *service;
	EAccount *acc;

	acc = (EAccount *) e_iterator_get (iter);
	service = acc->source;
	
	/* Iterate over valid accounts only */
	if (service->url && *service->url)
	        i++;

        e_iterator_next (iter);
	
    }

    return i;
}

QVariant EmailAccountListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

 //    QMailAccountId accountId = QMailAccountListModel::idFromIndex(index);
     EAccount *account;

     account = getAccountByIndex (index.row());
     
   //  QMailAccount account(accountId);
    if (role == DisplayName)
    {
        return QVariant(e_account_get_string(account, E_ACCOUNT_ID_NAME));
	//QMailAccountListModel::data(index, QMailAccountListModel::NameTextRole);
    }

    if (role == EmailAddress)
    {
        return QVariant(e_account_get_string(account, E_ACCOUNT_ID_ADDRESS));
    }

    if (role == MailServer)
    {
        QString address = QString(e_account_get_string(account, E_ACCOUNT_ID_ADDRESS));
        int index = address.indexOf("@");
        QString server = address.right(address.size() - index - 1);
        index = server.indexOf(".com", Qt::CaseInsensitive);
        return server.left(index);
    }

    if (role == UnreadCount)
    {
	return QVariant(1);
    }

    if (role == MailAccountId)
    {
        return QVariant(account->uid);
    }

    return QVariant();
}
/*
void EmailAccountListModel::onAccountsAdded(const QMailAccountIdList &ids)
{
    Q_UNUSED(ids);
    QMailAccountListModel::reset();
    emit accountAdded(QVariant(ids[0]));
}

void EmailAccountListModel::onAccountsRemoved(const QMailAccountIdList &ids)
{
    Q_UNUSED(ids);
    QMailAccountListModel::reset();
    emit accountRemoved(QVariant(ids[0]));
}

void EmailAccountListModel::onAccountsUpdated(const QMailAccountIdList &ids)
{
    Q_UNUSED(ids);
    QMailAccountListModel::reset();
}
*/

QVariant EmailAccountListModel::indexFromAccountId(QVariant id)
{
    int idx=0;
    QString sid = id.toString();
    

    if (id == 0)
        return idx;
    
    idx = getIndexById ((char *)sid.toLocal8Bit().constData());
    return idx;
}

QVariant EmailAccountListModel::getDisplayNameByIndex(int idx)
{
    return data(index(idx), EmailAccountListModel::DisplayName);
}

QVariant EmailAccountListModel::getEmailAddressByIndex(int idx)
{
    return data(index(idx), EmailAccountListModel::EmailAddress);
}

int EmailAccountListModel::getRowCount()
{
    return rowCount();
}

QVariant EmailAccountListModel::getAccountList()
{
    QStringList accountList;
    for (int row = 0; row < rowCount(); row++)
    {
        QString displayName = data(index(row), EmailAccountListModel::DisplayName).toString();
	accountList << displayName;
    }

    return accountList;
}

QVariant EmailAccountListModel::getAccountIdByIndex(int idx)
{
    return data(index(idx), EmailAccountListModel::MailAccountId);
}

void EmailAccountListModel::addPassword(QString key, QString password)
{
	const char *skey, *spass;
	
	//Handle Cancel password well. Like don't reprompt again in the same session/operation
	skey = key.toLocal8Bit().constData();
	spass = password.toLocal8Bit().constData();
	qDebug() << "\n\nSave Password: "<< key;

	session_instance->addPassword (key, password, true);
}

void EmailAccountListModel::sendReceive()
{
	session_instance->sendReceive();
	emit sendReceiveBegin();
}

void EmailAccountListModel::cancelOperations()
{
	emit sendReceiveCompleted();
	session_instance->cancelOperations();
}



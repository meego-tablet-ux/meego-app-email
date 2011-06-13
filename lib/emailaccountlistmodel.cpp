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
#include <libedataserver/e-data-server-util.h>
#include <gconf/gconf-client.h>
#include "emailaccountlistmodel.h"
#include "dbustypes.h"
#include "e-gdbus-emailstore-proxy.h"

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

void EmailAccountListModel::setUnreadCount (QVariant id, int count)
{
	acc_unread[id.toString()] = count;
	onAccountsUpdated (id.toString());
}

void EmailAccountListModel::updateUnreadCount (EAccount *account)
{	
	QDBusObjectPath store_id;
	OrgGnomeEvolutionDataserverMailStoreInterface *proxy;
	CamelFolderInfoArrayVariant folderlist;
	int count=0;
	const char *url;
	char *folder_name = NULL;

	url = e_account_get_string (account, E_ACCOUNT_SOURCE_URL);
	if (!url || !*url)
		return;
	
	QDBusPendingReply<QDBusObjectPath> reply;
	if (strncmp (url, "pop:", 4) == 0)
		reply  = session_instance->getLocalStore();
	else
		reply  = session_instance->getStore (QString(url));
        reply.waitForFinished();
	if (reply.isError())
		return;
        store_id = reply.value();
	
	if (strncmp (url, "pop:", 4) == 0) {
		const char *email;

		email = e_account_get_string(account, E_ACCOUNT_ID_ADDRESS);
		folder_name = g_strdup_printf ("%s/Inbox", email);
	}

        proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									store_id.path(),
									QDBusConnection::sessionBus(), this);
	
	QDBusPendingReply<CamelFolderInfoArrayVariant> reply1 = proxy->getFolderInfo (QString(folder_name ? folder_name : ""), 
								CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
	reply1.waitForFinished();
	folderlist = reply1.value ();	
	/* For POP3, create the folder if itsn't available */
	if (folderlist.length() == 0 && strncmp (url, "pop:", 4) == 0) {
		QDBusPendingReply<CamelFolderInfoArrayVariant> reply2;
		/* Create folder first*/
		reply2 = proxy->createFolder ("", folder_name);
		reply2.waitForFinished();
		folderlist = reply2.value ();	
	}

    	foreach (CamelFolderInfoVariant fInfo, folderlist)
    	{
		if (fInfo.unread_count > 0)
			count+= fInfo.unread_count;
	}

	g_free (folder_name);		
	delete proxy;
	acc_unread.insert (QString(account->uid), count);
}

EmailAccountListModel::EmailAccountListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    qDebug() << "EmailAccountListModel constructor";
    QHash<int, QByteArray> roles;
    char *path = g_build_filename (e_get_user_cache_dir(), "tmp", NULL);

    g_mkdir_with_parents (path, 0700);
    g_free (path);

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

    connect (session_instance, SIGNAL(AccountAdded(const QString &)), this,
             SLOT(onAccountsAdded (const QString &)));
    connect (session_instance, SIGNAL(AccountRemoved(const QString &)), this,
             SLOT(onAccountsRemoved(const QString &)));
    connect (session_instance, SIGNAL(AccountChanged (const QString &)), this,
             SLOT(onAccountsUpdated(const QString &)));
/*
    QMailAccountListModel::setSynchronizeEnabled(true);
    QMailAccountListModel::setKey(QMailAccountKey::messageType(QMailMessage::Email));
*/

    EIterator *iter;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
        account = (EAccount *) e_iterator_get (iter);
	updateUnreadCount(account);
        e_iterator_next (iter);
    }

    g_object_unref (iter);

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
	return acc_unread[QString(account->uid)];
    }

    if (role == MailAccountId)
    {
        return QVariant(account->uid);
    }

    return QVariant();
}

void EmailAccountListModel::onAccountsAdded(const QString &uid)
{
    qDebug() << uid + ": Account added";
    emit accountAdded(QVariant(uid));
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    endInsertRows();

}

void EmailAccountListModel::onAccountsRemoved(const QString &uid)
{
    int idx;
    char *cid = g_strdup((char *)uid.toLocal8Bit().constData());

    idx = getIndexById (cid);
    qDebug() << uid + ": Account removed";
    beginRemoveRows (QModelIndex(), idx, idx);
    endRemoveRows ();
    g_free (cid);
    emit accountRemoved(QVariant(uid));
}

void EmailAccountListModel::onAccountsUpdated(const QString &uid)
{
    char *cid = g_strdup((char *)uid.toLocal8Bit().constData());
    QModelIndex idx = createIndex (getIndexById(cid), 0);
    emit dataChanged(idx, idx);
    qDebug() << uid + ": Account changed";
    g_free (cid);
}


QVariant EmailAccountListModel::indexFromAccountId(QVariant id)
{
    int idx=0;
    QString sid = id.toString();
    char *cid = g_strdup((char *)sid.toLocal8Bit().constData());

    if (id == 0)
        return idx;
    
    idx = getIndexById (cid);
    g_free (cid);
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

QVariant EmailAccountListModel::getAllDisplayNames()
{
    QStringList displayNameList;
    for (int row = 0; row < rowCount(); row++)
    {
        QString displayName = data(index(row), EmailAccountListModel::DisplayName).toString();
        displayNameList << displayName;
    }
    return displayNameList;
}

QVariant EmailAccountListModel::getAllEmailAddresses()
{
    QStringList emailAddressList;
    for (int row = 0; row < rowCount(); row++)
    {
        QString emailAddress = data(index(row), EmailAccountListModel::EmailAddress).toString();
        emailAddressList << emailAddress;
    }
    return emailAddressList;
}

QVariant EmailAccountListModel::getAccountIdByIndex(int idx)
{
    return data(index(idx), EmailAccountListModel::MailAccountId);
}

void EmailAccountListModel::addPassword(QString key, QString password)
{
	char *skey, *spass;
	
	//Handle Cancel password well. Like don't reprompt again in the same session/operation
	skey = g_strdup(key.toLocal8Bit().constData());
	spass = g_strdup(password.toLocal8Bit().constData());
	qDebug() << "\n\nSave Password: "<< key;

	session_instance->addPassword (key, password, true);
	g_free (skey);
	g_free (spass);
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



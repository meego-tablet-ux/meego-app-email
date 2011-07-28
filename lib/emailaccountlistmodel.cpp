/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

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
   int i=0;
   EAccount * account;

   /* Update unread count once send recv is done. */
   for (i=0; i<mAcccountList.count(); i++) {
	EAccountService *service;

        account = mAcccountList[i];
	service = account->source;

	if (service->url && *service->url && account->enabled) {
		updateUnreadCount(account);
		onAccountsUpdated (QString(account->uid));
	}
    }
}

void EmailAccountListModel::setUnreadCount (QVariant id, int count)
{
	acc_unread[id.toString()] = count;
	onAccountsUpdated (id.toString());
}

QString createChecksum(QString uri)
{
	GChecksum *checksum;
	guint8 *digest;
	gsize length;
	char buffer[9];
	gint state = 0, save = 0;
	QString hash;

	length = g_checksum_type_get_length (G_CHECKSUM_MD5);
	digest = (guint8 *)g_alloca (length);

	checksum = g_checksum_new (G_CHECKSUM_MD5);
	g_checksum_update  (checksum, (const guchar *)uri.toLocal8Bit().constData(), -1);

	g_checksum_get_digest (checksum, digest, &length);
	g_checksum_free (checksum);

	g_base64_encode_step (digest, 6, FALSE, buffer, &state, &save);
	g_base64_encode_close (FALSE, buffer, &state, &save);
	buffer[8] = 0;
	
	hash = QString ((const char *)buffer);
	
    	return hash;
    
}

void EmailAccountListModel::updateUnreadCount (EAccount *account)
{	
	QDBusObjectPath store_id;
	OrgGnomeEvolutionDataserverMailStoreInterface *proxy;
	CamelFolderInfoArrayVariant folderlist;
	int count=0;
	const char *url;
	char *folder_name = NULL;
	const char *email = NULL;
	
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
		email = e_account_get_string(account, E_ACCOUNT_ID_ADDRESS);
	}

        proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									store_id.path(),
									QDBusConnection::sessionBus(), this);
	
	QDBusPendingReply<CamelFolderInfoArrayVariant> reply1 = proxy->getFolderInfo (QString(email ? email: ""), 
								CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
	reply1.waitForFinished();
	folderlist = reply1.value ();	
	/* For POP3, create the folder if itsn't available */
	if (folderlist.length() == 0 && strncmp (url, "pop:", 4) == 0) {
		QDBusPendingReply<CamelFolderInfoArrayVariant> reply2;

		/* Create base first*/
		folder_name = g_strdup_printf ("%s/Inbox", email);
		reply2 = proxy->createFolder ("", folder_name);
		reply2.waitForFinished();
		g_free (folder_name);		
		folder_name = g_strdup_printf ("%s/Drafts", email);
		reply2 = proxy->createFolder ("", folder_name);
		reply2.waitForFinished();

		g_free (folder_name);		
		folder_name = g_strdup_printf ("%s/Sent", email);
		reply2 = proxy->createFolder ("", folder_name);
		reply2.waitForFinished();
		g_free (folder_name);

		reply2 = proxy->getFolderInfo (QString(email), 
						CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
		folderlist = reply2.value ();	
		folderlist.removeLast();
	} else if (folderlist.length() ) {
		folderlist.removeLast();
	}

    	foreach (CamelFolderInfoVariant fInfo, folderlist)
    	{	
		bool loaded = folderLoaded[fInfo.uri];
		if (!fInfo.uri.isEmpty() && !loaded) {
			/* Create checksum and load its hierarchy into for remote loading */
			QString hash = createChecksum (fInfo.uri);
			accountUuid.insert (hash, QString(account->uid));
			folderUuid.insert (hash, QString(fInfo.uri));
			folderLoaded.insert(fInfo.uri, true);
			qDebug() << fInfo.uri + " " + QString(account->uid) + " " + hash;
		}

		if (fInfo.unread_count > 0)
			count+= fInfo.unread_count;
	}

	delete proxy;
	acc_unread.insert (QString(account->uid), count);
}

QVariant EmailAccountListModel::getAccountByUuid (QString uuid)
{
	QString hash = uuid.left(8);

	qDebug() << "getAccUuid " + uuid + "  " + hash;

	return QVariant(accountUuid[hash]);
}

QVariant EmailAccountListModel::getFolderByUuid (QString uuid)
{
	QString hash = uuid.left(8);		
	
	qDebug() << "getFolderUuid " + uuid + "  " + hash;

	return QVariant(folderUuid[hash]);
}

EmailAccountListModel::EmailAccountListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    qDebug() << "EmailAccountListModel constructor";
    QHash<int, QByteArray> roles;
    char *path;
    EIterator *iter;
    EAccount *account = NULL, *account_copy = NULL;
    int i;

    /* Inits glib for Content aggregator and mail app */
    g_type_init ();

    path = g_build_filename (e_get_user_cache_dir(), "tmp", NULL);

    g_mkdir_with_parents (path, 0700);
    g_free (path);

    registerMyDataTypes ();

    roles.insert(DisplayName, "displayName");
    roles.insert(EmailAddress, "emailAddress");
    roles.insert(MailServer, "mailServer");
    roles.insert(UnreadCount, "unreadCount");
    roles.insert(MailAccountId, "mailAccountId");
    setRoleNames(roles);

    accountUuid.clear();
    folderUuid.clear();
    folderLoaded.clear();
    acc_unread.clear();

    GConfClient *client;

    client = gconf_client_get_default ();
    account_list = e_account_list_new (client);
    g_object_unref (client);

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
	char *xml;
	account = (EAccount *) e_iterator_get (iter);
	xml = e_account_to_xml (account);
	account_copy = e_account_new_from_xml (xml);
	g_free (xml);
	mAcccountList.append (account_copy);
        e_iterator_next (iter);
    }

    g_object_unref (iter);
    account = NULL;

    /* Init here for password prompt */
    session_instance = new OrgGnomeEvolutionDataserverMailSessionInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                               QString ("/org/gnome/evolution/dataserver/Mail/Session"),
                                                               QDBusConnection::sessionBus(), parent);
    QObject::connect (session_instance, SIGNAL(GetPassword(const QString &, const QString &, const QString &)), this, SLOT(onGetPassword (const QString &, const QString &, const QString &)));
    QObject::connect (session_instance, SIGNAL(sendReceiveComplete()), this, SLOT(onSendReceiveComplete()));

    connect (session_instance, SIGNAL(AccountAdded(const QString &)), this,
             SLOT(onAccountsAdded (const QString &)));
    connect (session_instance, SIGNAL(AccountRemoved(const QString &)), this,
             SLOT(onAccountsRemoved(const QString &)));
    connect (session_instance, SIGNAL(AccountChanged (const QString &)), this,
             SLOT(onAccountsUpdated(const QString &)));

    /* Do a dummy call to wake up the daemon. Solves a whole bunch of issues. */
    session_instance->findPassword (QString("test"));
/*
    QMailAccountListModel::setSynchronizeEnabled(true);
    QMailAccountListModel::setKey(QMailAccountKey::messageType(QMailMessage::Email));
*/


    for (i=0; i<mAcccountList.count(); i++) {
	EAccountService *service;

        account = mAcccountList[i];
	service = account->source;

	if (service->url && *service->url && account->enabled)
		updateUnreadCount(account);
    }

}

EmailAccountListModel::~EmailAccountListModel()
{
    delete session_instance;
    g_object_unref (account_list);
    for (int i=0; i<mAcccountList.count(); i++) {
        g_object_unref (mAcccountList[i]);
    }
}

EAccount * EmailAccountListModel::getAccountByIndex(int idx) const
{
    int index=-1;
    EAccount *account = NULL;
    int i;

    for (i=0; i<mAcccountList.count(); i++) {
	EAccountService *service;

	account = mAcccountList[i];
	service = account->source;
	
	/* Iterate over valid accounts only */
	if (service->url && *service->url && account->enabled) {
	        index++;
	}
	
	if (index == idx)
		break;
    }

    return account;
}

EAccount * EmailAccountListModel::getAccountById(char *id)
{
    EAccount *account = NULL;
    int i;

    for (i=0; i<mAcccountList.count(); i++) {
        account = mAcccountList[i];
	if (strcmp (id, account->uid) == 0)
	     return account;
    }

    return NULL;
}

int EmailAccountListModel::getIndexById(char *id)
{
    EAccount *account = NULL;
    int index=-1;
    int i;

    for (i=0; i<mAcccountList.count(); i++) {
	EAccountService *service;

	account = mAcccountList[i];
	service = account->source;
	
	/* Iterate over valid accounts only */
	if (service->url && *service->url && account->enabled)
        	index++;

        account = mAcccountList[i];
        if (strcmp (id, account->uid) == 0)
             return index;

    }

    return -1;
}


int EmailAccountListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int index=0;
    int i;

    for (i=0; i<mAcccountList.count(); i++) {
	EAccountService *service;
	EAccount *acc;

	acc = mAcccountList[i];
	service = acc->source;
	
	/* Iterate over valid accounts only */
	if (service->url && *service->url && acc->enabled)
	        index++;

    }

    return index;
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
        return QVariant(QString::fromUtf8(e_account_get_string(account, E_ACCOUNT_ID_NAME)));
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
    EAccount *account, *account_copy;

    // Maintain a second list and manipulate. This might represent wrong data at some instance and can crash. 
    qDebug() << uid + ": Account added";
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    EIterator *iter = e_list_get_iterator (E_LIST (account_list));

    while (e_iterator_is_valid (iter)) {
	char *xml;
	account = (EAccount *) e_iterator_get (iter);
	if (uid == QString(account->uid)) {
		xml = e_account_to_xml (account);
		account_copy = e_account_new_from_xml (xml);
		g_free (xml);
		mAcccountList.append (account_copy);
		break;
	}
        e_iterator_next (iter);
    }
    g_object_unref (iter);
    endInsertRows();

    emit accountAdded(QVariant(uid));
}

void EmailAccountListModel::onAccountsRemoved(const QString &uid)
{
    // Deal with a better mapping than a hack.
    int index;
    char *cid = g_strdup((char *)uid.toLocal8Bit().constData());
    EAccount *account;

    index = getIndexById (cid);
    beginRemoveRows (QModelIndex(), index, index);
    account = mAcccountList[index];
    mAcccountList.removeAt (index);
    g_object_unref (account);
    endRemoveRows ();
    emit accountRemoved(QVariant(uid));

    g_free (cid);
}

void EmailAccountListModel::onAccountsUpdated(const QString &uid)
{
    char *cid = g_strdup((char *)uid.toLocal8Bit().constData());
    int index = getIndexById(cid);
    QModelIndex idx;
    int newidx = -1;

    qDebug() << uid + ": Account changed at index ";
    printf("at index %d\n", index);

    EIterator *iter = e_list_get_iterator (E_LIST (account_list));
    EAccount *account = NULL, *copy=NULL;
    while (e_iterator_is_valid (iter)) {
	char *xml;
	
	newidx++;
	account = (EAccount *) e_iterator_get (iter);
	if (uid == QString(account->uid)) {
		xml = e_account_to_xml (account);
		copy = e_account_new_from_xml (xml);
		g_free (xml);
		break;
	}
        e_iterator_next (iter);
    }
    g_object_unref (iter);
    if (!copy) {
	g_warning ("Unable to find %sin the account list\n", cid);
	return;
    }
    account = getAccountById (cid);
    if (!copy->enabled && account->enabled) {
	//int newidx;
	//newidx = mAcccountList.indexOf (account);
	/* Account disabled, so lets remove from the view. */
	idx = createIndex (index, 0);
	beginRemoveRows (QModelIndex(), index, index);
	mAcccountList[newidx] = copy;
	endRemoveRows ();
	qDebug() << "Disabling account";
	printf("At %d/%d at %d\n", newidx, mAcccountList.count(), index);
    } else if (copy->enabled && !account->enabled) {
	//int newidx;
	//newidx = mAcccountList.indexOf (account);
	/* An account just enabled, insert that into the view. */
	account->enabled = TRUE;
	index = getIndexById (cid);
	idx = createIndex (index, 0);
	beginInsertRows(QModelIndex(), index, index);
	mAcccountList[newidx] = copy;
	endInsertRows();
	qDebug() << "Enabling account";
	printf("At %d/%d at %d\n", newidx, mAcccountList.count(), index);
    } else {
	/* Some other thigns just changed, emit a dataChanged */
    	idx = createIndex (index, 0);
	mAcccountList[index] = copy;
    	emit dataChanged(idx, idx);
    }
	
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

QVariant EmailAccountListModel::getSignatureForAccount (QString id)
{
    EAccount *account = getAccountById ((char *)id.toLocal8Bit().constData());

    return QVariant ( QString::fromUtf8 (e_account_get_string(account, E_ACCOUNT_ID_SIGNATURE)));
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



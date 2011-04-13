/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "folderlistmodel.h"
#include <QMailAccount>
#include <QMailFolder>
#include <QMailMessage>
#include <QMailMessageKey>
#include <QMailStore>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <qmailnamespace.h>
#include <libedataserver/e-account-list.h>
#include <libedataserver/e-list.h>
#include <gconf/gconf-client.h>

FolderListModel::FolderListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles.insert(FolderName, "folderName");
    roles.insert(FolderId, "folderId");
    roles.insert(FolderUnreadCount, "folderUnreadCount");
    roles.insert(FolderServerCount, "folderServerCount");
    setRoleNames(roles);
}

FolderListModel::~FolderListModel()
{
}

int FolderListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_folderlist.count();
}

QVariant FolderListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() > m_folderlist.count())
        return QVariant();

    
    CamelFolderInfoVariant  folder(m_folderlist[index.row()]);
    if (role == FolderName)
    {
        return QVariant(folder.full_name);
    }
    else if (role == FolderId)
    {
        return QVariant(folder.uri);
    } 
    else if (role == FolderUnreadCount)
    {
        return QVariant ((folder.unread_count == -1) ? 0 : folder.unread_count );
    }
    else if (role == FolderServerCount)
    {
        return QVariant ((folder.total_mail_count == -1) ? 0 : folder.total_mail_count);
    }

    return QVariant();
}

EAccount * FolderListModel::getAccountById(EAccountList *account_list, char *id)
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

void FolderListModel::setAccountKey(QVariant id)
{
    GConfClient *client;
    EAccountList *account_list;
    QString quid;
    const char *url;

    client = gconf_client_get_default ();
    account_list = e_account_list_new (client);
    g_object_unref (client);

    quid = id.value<QString>();    
    m_account = getAccountById (account_list, (char *)quid.toLocal8Bit().constData());
    url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);

    g_print ("fetching store: %s\n", url);
    OrgGnomeEvolutionDataserverMailSessionInterface *instance = OrgGnomeEvolutionDataserverMailSessionInterface::instance(this);
    if (instance && instance->isValid()) {
	QDBusPendingReply<QDBusObjectPath> reply = instance->getStore (QString(url));
        reply.waitForFinished();
        m_store_proxy_id = reply.value();
	g_print ("Store PATH: %s\n", (char *) m_store_proxy_id.path().toLocal8Bit().constData());
        m_store_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									m_store_proxy_id.path(),
									QDBusConnection::sessionBus(), this);
	
	if (m_store_proxy && m_store_proxy->isValid()) {
		QDBusPendingReply<CamelFolderInfoArrayVariant> reply = m_store_proxy->getFolderInfo (QString(""), 
									CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
		reply.waitForFinished();
		m_folderlist = reply.value ();	
	g_print ("Got folder list....\n");

	}
    }
}

QStringList FolderListModel::folderNames()
{
    QStringList folderNames;

    foreach (CamelFolderInfoVariant fInfo, m_folderlist)
    {
        QString displayName = QString (fInfo.full_name);

        if (fInfo.unread_count > 0)
        {
            displayName = displayName + " (" + QString::number(fInfo.unread_count) + ")";
        }
	
        folderNames << displayName;
	g_print("FOLDER: %s\n", (char *)displayName.toLocal8Bit().constData());
    }
    return folderNames;
}

QVariant FolderListModel::folderId(int index)
{
    if (index < 0 || index >= m_folderlist.count())
        return QVariant();
   
    return m_folderlist[index].uri;
}

int FolderListModel::indexFromFolderId(QVariant vFolderId)
{   
    
    QString uri = vFolderId.toString();
    g_print ("Entering FolderListModel::indexFromFolderId: %s\n", (char *)uri.toLocal8Bit().constData());
    for (int i = 0; i < m_folderlist.size(); i ++)
    {
        if (vFolderId == 0) {
	        CamelFolderInfoVariant folder(m_folderlist[i]);
        	if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0)
			return i;

	} else {
        	if (uri == m_folderlist[i].uri)
            		return i;
	}
    }
    return -1;
}

QVariant FolderListModel::folderServerCount(QVariant vFolderId)
{
    QString uri = vFolderId.toString();

    for (int i = 0; i < m_folderlist.size(); i ++)
    {
        if (uri == m_folderlist[i].uri)
            return QVariant(m_folderlist[i].total_mail_count);
    }
    return QVariant(-1);
}

QVariant FolderListModel::inboxFolderId()
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);
        if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0) {
	    g_print ("Returning INBOX URI: %s\n", (char *)folder.uri.toLocal8Bit().constData());
            return QVariant(folder.uri);
	}
    }

    return QVariant();
}

QVariant FolderListModel::inboxFolderName()
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);

       if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0) {
            g_print ("Returning INBOX URI: %s\n", (char *)folder.uri.toLocal8Bit().constData());
            return QVariant(folder.folder_name);
        }
    }
    return QVariant("");
}

int FolderListModel::totalNumberOfFolders()
{
    return m_folderlist.size();
}

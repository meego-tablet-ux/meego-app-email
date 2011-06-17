/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef FOLDERLISTMODEL_H
#define FOLDERLISTMODEL_H


#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif

#include "dbustypes.h"
#include <QAbstractListModel>
#include <libedataserver/e-account-list.h>
#include <libedataserver/e-account.h>
#include <gconf/gconf-client.h>
#include "e-gdbus-emailsession-proxy.h"
#include "e-gdbus-emailstore-proxy.h"
#include "e-gdbus-emailfolder-proxy.h"

class FolderListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FolderListModel (QObject *parent = 0);
    ~FolderListModel();

    enum Role {
        FolderName = Qt::UserRole + 1,
        FolderId = Qt::UserRole + 2,
        FolderUnreadCount = Qt::UserRole + 3,
        FolderServerCount = Qt::UserRole + 4,
        Index = Qt::UserRole + 5,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    Q_INVOKABLE void setAccountKey (QVariant id);
    Q_INVOKABLE QStringList folderNames();
    Q_INVOKABLE QVariant folderId(int index);
    Q_INVOKABLE int indexFromFolderId(QVariant vFolderId);
    Q_INVOKABLE QVariant folderServerCount(QVariant vFolderId);
    Q_INVOKABLE QVariant inboxFolderId ();
    Q_INVOKABLE QVariant inboxFolderName();
    Q_INVOKABLE void createFolder(const QString &name, QVariant parentFolderId);
    Q_INVOKABLE void deleteFolder(QVariant folderId);
    Q_INVOKABLE void renameFolder(QVariant folderId, const QString &name);
    Q_INVOKABLE int getFolderMailCount ();

    Q_INVOKABLE int totalNumberOfFolders();
    Q_INVOKABLE int saveDraft(const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, bool html, const QStringList &attachment_uris, int priority);
    Q_INVOKABLE int sendMessage(const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, bool html, const QStringList &attachment_uris, int priority); 




private:
    CamelFolderInfoArrayVariant m_folderlist;
    char *pop_foldername;
    EAccount * getAccountById(EAccountList *account_list, char *id);
    EAccount *m_account;
    QDBusObjectPath m_store_proxy_id;
    OrgGnomeEvolutionDataserverMailStoreInterface *m_store_proxy;
    
    QDBusObjectPath m_outbox_proxy_id;
    OrgGnomeEvolutionDataserverMailFolderInterface *m_outbox_proxy;
    QDBusObjectPath m_sent_proxy_id;
    OrgGnomeEvolutionDataserverMailFolderInterface *m_sent_proxy;
    QDBusObjectPath m_drafts_proxy_id;
    OrgGnomeEvolutionDataserverMailFolderInterface *m_drafts_proxy;

};

#endif

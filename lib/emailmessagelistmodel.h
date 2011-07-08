/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef EMAILMESSAGELISTMODEL_H
#define EMAILMESSAGELISTMODEL_H

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif

#include <QAbstractListModel>
#include <QProcess>
#include <libedataserver/e-account.h>
#include "dbustypes.h"
#include "e-gdbus-emailsession-proxy.h"
#include "e-gdbus-emailstore-proxy.h"
#include "e-gdbus-emailfolder-proxy.h"

class EmailMessageListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum SortBy
    {
	SortSubject = 1,
	SortSender,
	SortDate,
	SortAttachment
    };

    enum Roles
    {
        MessageAddressTextRole = Qt::UserRole,
        MessageSubjectTextRole,
        MessageFilterTextRole,
        MessageTimeStampTextRole,
        MessageSizeTextRole,
        MessageTypeIconRole,
        MessageStatusIconRole,
        MessageDirectionIconRole,
        MessagePresenceIconRole,
        MessageBodyTextRole,
        MessageIdRole,
        MessageAttachmentCountRole, // returns number of attachment
        MessageAttachmentsRole,                                // returns a list of attachments
        MessageRecipientsRole,                                 // returns a list of recipients (email address)
        MessageRecipientsDisplayNameRole,                      // returns a list of recipients (displayName)
        MessageReadStatusRole,                                 // returns the read/unread status
        MessageQuotedBodyRole,                                 // returns the quoted body
        MessageHtmlBodyRole,                                   // returns the html body
        MessageUuidRole,			               // returns a unique string id
        MessageSenderDisplayNameRole,                          // returns sender's display name
        MessageSenderEmailAddressRole,                         // returns sender's email address
        MessageCcRole,                                         // returns a list of Cc (email + displayName)
        MessageBccRole,                                        // returns a list of Bcc (email + displayName)
        MessageTimeStampRole,                                  // returns timestamp in QDateTime format
        MessageSelectModeRole                                  // returns the select mode
    };


    EmailMessageListModel (QObject *parent = 0);
    ~EmailMessageListModel();
    int rowCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant mydata (int row, int role = Qt::DisplayRole) const;    
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    QString bodyText(QString&, bool plain) const;

    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &idx) const;
    int columnCount(const QModelIndex& idx) const;

    QModelIndex generateIndex(int row, int column, void *ptr);

    

signals:
    void messageDownloadCompleted();
    void messageRetrievalCompleted();
    void sendReceiveCompleted ();
    void sendReceiveBegin ();
    void folderChanged ();

public slots:
    Q_INVOKABLE void setFolderKey (QVariant id);
    Q_INVOKABLE void setAccountKey (QVariant id);
    Q_INVOKABLE void sortBySender (int key);
    Q_INVOKABLE void sortBySubject (int key);
    Q_INVOKABLE void sortByDate (int key);
    Q_INVOKABLE void sortByAttachment (int key);
    Q_INVOKABLE void setSearch(const QString search);

    Q_INVOKABLE QVariant indexFromMessageId(QString msgId);
    Q_INVOKABLE QVariant messageId (int index);
    Q_INVOKABLE QVariant subject (int index);
    Q_INVOKABLE QVariant mailSender (int index);
    Q_INVOKABLE QVariant timeStamp (int index);
    Q_INVOKABLE QVariant body (int index);
    Q_INVOKABLE QVariant htmlBody (int index);
    Q_INVOKABLE QVariant quotedBody (int index);
    Q_INVOKABLE QVariant attachments (int index);
    Q_INVOKABLE QVariant numberOfAttachments (int index);
    Q_INVOKABLE QVariant recipients (int index);
    Q_INVOKABLE QVariant ccList (int index);
    Q_INVOKABLE QVariant bccList (int index);
    Q_INVOKABLE QVariant toList (int index);
    Q_INVOKABLE QVariant messageRead (int index);
    Q_INVOKABLE int messagesCount ();
    Q_INVOKABLE int totalCount ();
    Q_INVOKABLE bool stillMoreMessages ();
    Q_INVOKABLE void deSelectAllMessages();
    Q_INVOKABLE void selectMessage( int index );
    Q_INVOKABLE void deSelectMessage (int index );
    Q_INVOKABLE void moveSelectedMessageIds(QVariant vFolderId);
    Q_INVOKABLE void deleteSelectedMessageIds();
    Q_INVOKABLE void deleteMessage(QVariant id);
    Q_INVOKABLE void deleteMessages(QList<QString>);
    Q_INVOKABLE void markMessageAsRead (QVariant id);
    Q_INVOKABLE void markMessageAsUnread (QVariant id);
    Q_INVOKABLE void saveAttachment (int row, QString uri);
    Q_INVOKABLE bool openAttachment (int row, QString uri);
    Q_INVOKABLE void getMoreMessages ();
    Q_INVOKABLE void saveAttachmentIn (int row, QString uri, bool tmp);
    Q_INVOKABLE void saveAttachmentsInTemp (int row);
    Q_INVOKABLE void sendReceive ();
    Q_INVOKABLE void cancelOperations();


private slots:
    //ckw void downloadActivityChanged(QMailServiceAction::Activity);
    void myFolderChanged(const QStringList &added, const QStringList &removed, const QStringList &changed, const QStringList &recent);
    void updateSearch ();
    void onSendReceiveComplete ();
    void fetchMoreMessagesFinished (QDBusPendingCallWatcher *call);

private:
    void initMailServer ();
    void createChecksum ();
    void sortMails ();
    void setMessageFlag (QString uid, uint flag, uint set);
    void reloadFolderUids ();
    void loadMessages (int);
    void loadMoreMessages (int);
    QString mimeMessage (QString &uid);

    CamelFolderInfoArrayVariant m_folders; 
    QStringList folder_uids;
    QStringList shown_uids;
    QStringList folder_vuids;
    QTimer *timer;
    QString search_str;
    bool messages_present;

    QString accout_id;
    QString m_current_folder;
    QString m_current_hash;
    QHash<QString, CamelMessageInfoVariant> m_infos;
    QHash<QString ,QString> *m_messages;
    EAccount *m_account;
    QDBusObjectPath m_store_proxy_id;
    QDBusObjectPath m_lstore_proxy_id;
    OrgGnomeEvolutionDataserverMailStoreInterface *m_store_proxy;
    OrgGnomeEvolutionDataserverMailStoreInterface *m_lstore_proxy;
    QDBusObjectPath m_folder_proxy_id;
    OrgGnomeEvolutionDataserverMailFolderInterface *m_folder_proxy;
    OrgGnomeEvolutionDataserverMailSessionInterface *session_instance;
  
    QProcess m_msgAccount;
    QProcess m_messageServerProcess;
    QString m_search;
    EmailMessageListModel::SortBy m_sortById;
    int m_sortKey;
    QList<QString> m_selectedMsgIds;
};

#endif

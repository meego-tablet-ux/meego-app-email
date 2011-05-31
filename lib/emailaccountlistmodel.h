/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef EMAILACCOUNTLISTMODEL_H
#define EMAILACCOUNTLISTMODEL_H

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif

#include <QAbstractListModel>
#include <QMailAccount>
#include "e-gdbus-emailsession-proxy.h"

#include <libedataserver/e-account-list.h>
#include <gconf/gconf-client.h>

class EmailAccountListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit EmailAccountListModel (QObject *parent = 0);
    ~EmailAccountListModel();

    enum Role {
        DisplayName = Qt::UserRole,
        EmailAddress, 
        MailServer, 
        UnreadCount,
        MailAccountId,
        Index,
    };

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

public slots:
    Q_INVOKABLE QVariant indexFromAccountId(QVariant id);
    Q_INVOKABLE QVariant getDisplayNameByIndex(int idx);
    Q_INVOKABLE QVariant getEmailAddressByIndex(int idx);
    Q_INVOKABLE int getRowCount();
    Q_INVOKABLE QVariant getAllEmailAddresses();
    Q_INVOKABLE QVariant getAllDisplayNames();
    Q_INVOKABLE QVariant getAccountIdByIndex(int idx);
    Q_INVOKABLE void addPassword(QString key, QString password);
    Q_INVOKABLE void sendReceive();
    Q_INVOKABLE void cancelOperations();

signals:
    void accountAdded(QVariant accountId);
    void accountRemoved(QVariant accountId);
    void askPassword (QString title, QString prompt, QString key);
    void sendReceiveCompleted ();
    void sendReceiveBegin ();
    void modelReset();
private:
    OrgGnomeEvolutionDataserverMailSessionInterface *session_instance;
    EAccountList *account_list;
    QHash<QString, int> acc_unread;
    void updateUnreadCount (EAccount *account);

private slots:
    EAccount * getAccountByIndex (int idx) const;
    EAccount * getAccountById(char *id);
    int getIndexById(char *id);
    void onGetPassword (const QString &, const QString &, const QString &);
    void onSendReceiveComplete ();
    void onAccountsAdded(const QString &);
    void onAccountsRemoved(const QString &);
    void onAccountsUpdated(const QString &);


};

#endif

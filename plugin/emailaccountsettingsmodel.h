/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef EMAILACCOUNTSETTINGSMODEL_H
#define EMAILACCOUNTSETTINGSMODEL_H

#include <QAbstractListModel>
#include <mgconfitem.h>

#include <libedataserver/e-account.h>
#include <libedataserver/e-account-list.h>
#include "e-gdbus-emailsession-proxy.h"

class EmailAccountSettingsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum AccountListRoles {
        DescriptionRole = Qt::UserRole + 1,
        EnabledRole,
        NameRole,
        AddressRole,
        PasswordRole,

        RecvTypeRole,
        RecvServerRole,
        RecvPortRole,
        RecvSecurityRole,
        RecvUsernameRole,
        RecvPasswordRole,

        SendServerRole,
        SendPortRole,
        SendAuthRole,
        SendSecurityRole,
        SendUsernameRole,
        SendPasswordRole,

        PresetRole
    };
    EmailAccountSettingsModel(QObject *parent = 0);
    ~EmailAccountSettingsModel();

    Q_INVOKABLE void reload();
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    // wrappers to allow data, setData to be called from QML
    Q_INVOKABLE QVariant dataWrapper(int row, int role);
    Q_INVOKABLE bool setDataWrapper(int row, const QVariant &value, int role);

    Q_INVOKABLE int updateInterval();
    Q_INVOKABLE void setUpdateInterval(int interval);
    Q_INVOKABLE QString signature();
    Q_INVOKABLE void setSignature(QString signature);
    Q_INVOKABLE bool newMailNotifications();
    Q_INVOKABLE void setNewMailNotifications(bool val);
    Q_INVOKABLE bool confirmDeleteMail();
    Q_INVOKABLE void setConfirmDeleteMail(bool val);

public slots:
    void saveChanges();
    void deleteRow(int row);

private:
    QHash<int,QByteArray> roles;

    EAccountList *mAccountList;
    int mUpdateInterval;
    int mLength;
    QString mSignature;
    bool mNewMailNotification;
    bool mConfirmDeleteMail;
    MGConfItem *mUpdateIntervalConf;
    MGConfItem *mSignatureConf;
    MGConfItem *mNewMailNotificationConf;
    MGConfItem *mConfirmDeleteMailConf;
    OrgGnomeEvolutionDataserverMailSessionInterface *session;

    void init();
    EAccount *getAccountByIndex(int idx) const;
};

#endif // EMAILACCOUNTSETTINGSMODEL_H


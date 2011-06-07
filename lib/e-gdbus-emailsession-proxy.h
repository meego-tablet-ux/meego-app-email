/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -i dbustypes.h -p e-gdbus-emailsession-proxy /home/meego/git/evolution-data-server/mail/daemon/e-mail-data-session.xml org.gnome.evolution.dataserver.mail.Session
 *
 * qdbusxml2cpp is Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef E_GDBUS_EMAILSESSION_PROXY_H_1302137795
#define E_GDBUS_EMAILSESSION_PROXY_H_1302137795  

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>
#include "dbustypes.h"

/*
 * Proxy class for interface org.gnome.evolution.dataserver.mail.Session
 */
class OrgGnomeEvolutionDataserverMailSessionInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.evolution.dataserver.mail.Session"; }

public:
    OrgGnomeEvolutionDataserverMailSessionInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgGnomeEvolutionDataserverMailSessionInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> addPassword(const QString &key, const QString &password, bool remember)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(key) << qVariantFromValue(password) << qVariantFromValue(remember);
        return asyncCallWithArgumentList(QLatin1String("addPassword"), argumentList);
    }

    inline QDBusPendingReply<> cancelOperations()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("cancelOperations"), argumentList);
    }

    inline QDBusPendingReply<> fetchAccount(const QString &uid)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid);
        return asyncCallWithArgumentList(QLatin1String("fetchAccount"), argumentList);
    }

    inline QDBusPendingReply<bool> fetchOldMessages(const QString &uid, int count)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid) << qVariantFromValue(count);
        return asyncCallWithArgumentList(QLatin1String("fetchOldMessages"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> getFolderFromUri(const QString &uri)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uri);
        return asyncCallWithArgumentList(QLatin1String("getFolderFromUri"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> getLocalFolder(const QString &type)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(type);
        return asyncCallWithArgumentList(QLatin1String("getLocalFolder"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> getLocalStore()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getLocalStore"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> getStore(const QString &uri)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uri);
        return asyncCallWithArgumentList(QLatin1String("getStore"), argumentList);
    }

    static OrgGnomeEvolutionDataserverMailSessionInterface* instance(QObject *parent);

    inline QDBusPendingReply<> sendReceive()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("sendReceive"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void AccountAdded(const QString &uid);
    void AccountChanged(const QString &uid);
    void AccountRemoved(const QString &uid);
    void GetPassword(const QString &title, const QString &prompt, const QString &key);
    void sendReceiveComplete();
};

namespace org {
  namespace gnome {
    namespace evolution {
      namespace dataserver {
        namespace mail {
          typedef ::OrgGnomeEvolutionDataserverMailSessionInterface Session;
        }
      }
    }
  }
}
#endif

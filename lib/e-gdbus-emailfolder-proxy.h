/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -i dbustypes.h -p e-gdbus-emailfolder-proxy /home/meego/git/evolution-data-server/mail/daemon/e-mail-data-folder.xml org.gnome.evolution.dataserver.mail.Folder
 *
 * qdbusxml2cpp is Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef E_GDBUS_EMAILFOLDER_PROXY_H_1302131688
#define E_GDBUS_EMAILFOLDER_PROXY_H_1302131688 

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
 * Proxy class for interface org.gnome.evolution.dataserver.mail.Folder
 */
class OrgGnomeEvolutionDataserverMailFolderInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.evolution.dataserver.mail.Folder"; }

public:
    OrgGnomeEvolutionDataserverMailFolderInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgGnomeEvolutionDataserverMailFolderInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QString, bool> AppendMessage(CamelMessageInfoVariant info, const QString &message)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(info) << qVariantFromValue(message);
        return asyncCallWithArgumentList(QLatin1String("AppendMessage"), argumentList);
    }
    inline QDBusReply<QString> AppendMessage(CamelMessageInfoVariant info, const QString &message, bool &success)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(info) << qVariantFromValue(message);
        QDBusMessage reply = callWithArgumentList(QDBus::Block, QLatin1String("AppendMessage"), argumentList);
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == 2) {
            success = qdbus_cast<bool>(reply.arguments().at(1));
        }
        return reply;
    }

    inline QDBusPendingReply<int> deletedMessageCount()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("deletedMessageCount"), argumentList);
    }

    inline QDBusPendingReply<bool> expunge()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("expunge"), argumentList);
    }

    inline QDBusPendingReply<bool> fetchOldMessages(int count)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(count);
        return asyncCallWithArgumentList(QLatin1String("fetchOldMessages"), argumentList);
    }

    inline QDBusPendingReply<QString> getDescription()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getDescription"), argumentList);
    }

    inline QDBusPendingReply<QString> getFullName()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getFullName"), argumentList);
    }

    inline QDBusPendingReply<QString> getMessage(const QString &uid)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid);
        return asyncCallWithArgumentList(QLatin1String("getMessage"), argumentList);
    }

    inline QDBusPendingReply<uint> getMessageFlags(const QString &uid)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid);
        return asyncCallWithArgumentList(QLatin1String("getMessageFlags"), argumentList);
    }

    inline QDBusPendingReply<CamelMessageInfoVariant> getMessageInfo(const QString &uid)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid);
        return asyncCallWithArgumentList(QLatin1String("getMessageInfo"), argumentList);
    }

    inline QDBusPendingReply<bool> getMessageUserFlag(const QString &uid, const QString &flagname)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid) << qVariantFromValue(flagname);
        return asyncCallWithArgumentList(QLatin1String("getMessageUserFlag"), argumentList);
    }

    inline QDBusPendingReply<QString> getMessageUserTag(const QString &uid, const QString &param)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid) << qVariantFromValue(param);
        return asyncCallWithArgumentList(QLatin1String("getMessageUserTag"), argumentList);
    }

    inline QDBusPendingReply<QString> getName()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getName"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> getParentStore()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getParentStore"), argumentList);
    }

    inline QDBusPendingReply<uint> getPermanentFlags()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getPermanentFlags"), argumentList);
    }

    inline QDBusPendingReply<QStringList> getUids()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("getUids"), argumentList);
    }

    inline QDBusPendingReply<bool> hasSearchCapability()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("hasSearchCapability"), argumentList);
    }

    inline QDBusPendingReply<bool> hasSummaryCapability()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("hasSummaryCapability"), argumentList);
    }

    inline QDBusPendingReply<> prepareSummary()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("prepareSummary"), argumentList);
    }

    inline QDBusPendingReply<bool> refreshInfo()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("refreshInfo"), argumentList);
    }

    inline QDBusPendingReply<QStringList> searchByExpression(const QString &expression)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(expression);
        return asyncCallWithArgumentList(QLatin1String("searchByExpression"), argumentList);
    }

    inline QDBusPendingReply<QStringList> searchByUids(const QString &expression, const QStringList &searchuids)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(expression) << qVariantFromValue(searchuids);
        return asyncCallWithArgumentList(QLatin1String("searchByUids"), argumentList);
    }

    inline QDBusPendingReply<QStringList> searchSortByExpression(const QString &expression, const QString &sort, bool ascending)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(expression) << qVariantFromValue(sort) << qVariantFromValue(ascending);
        return asyncCallWithArgumentList(QLatin1String("searchSortByExpression"), argumentList);
    }

    inline QDBusPendingReply<> setDescription(const QString &desc)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(desc);
        return asyncCallWithArgumentList(QLatin1String("setDescription"), argumentList);
    }

    inline QDBusPendingReply<> setFullName(const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(name);
        return asyncCallWithArgumentList(QLatin1String("setFullName"), argumentList);
    }

    inline QDBusPendingReply<bool> setMessageFlags(const QString &uid, uint flags, uint set)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid) << qVariantFromValue(flags) << qVariantFromValue(set);
        return asyncCallWithArgumentList(QLatin1String("setMessageFlags"), argumentList);
    }

    inline QDBusPendingReply<> setMessageUserFlag(const QString &uid, const QString &flagname, uint set)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid) << qVariantFromValue(flagname) << qVariantFromValue(set);
        return asyncCallWithArgumentList(QLatin1String("setMessageUserFlag"), argumentList);
    }

    inline QDBusPendingReply<> setMessageUserTag(const QString &uid, const QString &param, const QString &value)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uid) << qVariantFromValue(param) << qVariantFromValue(value);
        return asyncCallWithArgumentList(QLatin1String("setMessageUserTag"), argumentList);
    }

    inline QDBusPendingReply<> setName(const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(name);
        return asyncCallWithArgumentList(QLatin1String("setName"), argumentList);
    }

    inline QDBusPendingReply<bool> sync(bool expunge)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(expunge);
        return asyncCallWithArgumentList(QLatin1String("sync"), argumentList);
    }

    inline QDBusPendingReply<int> totalMessageCount()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("totalMessageCount"), argumentList);
    }

    inline QDBusPendingReply<QStringList> transferMessagesTo(const QStringList &uids, const QDBusObjectPath &destfolder, bool deleteoriginals)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uids) << qVariantFromValue(destfolder) << qVariantFromValue(deleteoriginals);
        return asyncCallWithArgumentList(QLatin1String("transferMessagesTo"), argumentList);
    }

    inline QDBusPendingReply<int> unreadMessageCount()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("unreadMessageCount"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void FolderChanged(const QStringList &uids_added, const QStringList &uids_removed, const QStringList &uids_changed, const QStringList &uids_recent);
};

namespace org {
  namespace gnome {
    namespace evolution {
      namespace dataserver {
        namespace mail {
          typedef ::OrgGnomeEvolutionDataserverMailFolderInterface Folder;
        }
      }
    }
  }
}
#endif

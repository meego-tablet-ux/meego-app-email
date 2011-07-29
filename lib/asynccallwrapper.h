#ifndef ASYNCCALLWRAPPER_H
#define ASYNCCALLWRAPPER_H

#include "dbustypes.h"
//Qt
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtDBus/QDBusPendingCall>

class OrgGnomeEvolutionDataserverMailFolderInterface;
class OrgGnomeEvolutionDataserverMailStoreInterface;

class SearchSortByExpression: public QObject
{
    Q_OBJECT

public:
    explicit SearchSortByExpression(OrgGnomeEvolutionDataserverMailFolderInterface *folderProxy, QObject *parent = 0);

    virtual ~SearchSortByExpression() {}

    inline void setQuery(const QString& query) { mQuery = query; }
    inline void setSort(const QString& sort) { mSort = sort; }

public slots:
    void start();

signals:
    void result(const QStringList& uids);
    void finished();

private slots:
    void onAsyncCallFinished(QDBusPendingCallWatcher* mWatcher);

private:
    QString mQuery;
    QString mSort;
    OrgGnomeEvolutionDataserverMailFolderInterface* const mFolderProxy;
    QDBusPendingCallWatcher* mPrepareSumWatcher;
    QDBusPendingCallWatcher* mSearchWatcher;
};

class GetMessageInfo: public QObject
{
    Q_OBJECT

public:
    explicit GetMessageInfo(OrgGnomeEvolutionDataserverMailFolderInterface *folderProxy, QObject* parent = 0) : QObject(parent), mFolderProxy(folderProxy), mCount(0)
    { Q_ASSERT (mFolderProxy); }

    virtual ~GetMessageInfo() {}

    void setMessageUids(const QStringList& uids);

public slots:
    void start(int count = 0);

signals:
    void result(const CamelMessageInfoVariant& info);
    void finished();

private slots:
    void onAsyncCallFinished(QDBusPendingCallWatcher* watcher);

private:
    QStringList mUids;
    OrgGnomeEvolutionDataserverMailFolderInterface* const mFolderProxy;
    int mCount;
};

class GetFolder: public QObject
{
    Q_OBJECT

public:
    explicit GetFolder(OrgGnomeEvolutionDataserverMailStoreInterface *storeProxy, OrgGnomeEvolutionDataserverMailStoreInterface *lstoreProxy = 0, QObject* parent = 0)
        :QObject(parent), mStoreProxy(storeProxy), mLstoreProxy(lstoreProxy) {}
    virtual ~GetFolder() {}

    void setFolderName(const QString& folderName);
    void setKnownFolders(const CamelFolderInfoArrayVariant& folders);

public slots:
    void start();

signals:
    void result(const QString& folderName, const QString& objectPath);
    void finished();

private slots:
    void onAsyncCallFinished(QDBusPendingCallWatcher *watcher);

private:
    QString mFolderName;
    OrgGnomeEvolutionDataserverMailStoreInterface *mStoreProxy;
    OrgGnomeEvolutionDataserverMailStoreInterface *mLstoreProxy;
    CamelFolderInfoArrayVariant mKnownFolders;
};

#endif // ASYNCCALLWRAPPER_H

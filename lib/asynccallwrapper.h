#ifndef ASYNCCALLWRAPPER_H
#define ASYNCCALLWRAPPER_H

#include "dbustypes.h"
//Qt
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtDBus/QDBusPendingCall>
//eds
#include <libedataserver/e-account-list.h>

class OrgGnomeEvolutionDataserverMailFolderInterface;
class OrgGnomeEvolutionDataserverMailStoreInterface;
class OrgGnomeEvolutionDataserverMailSessionInterface;

class AsyncOperation : public QObject
{
    Q_OBJECT

public:
    explicit AsyncOperation(QObject *parent = 0);
    virtual ~AsyncOperation() {}

signals:
    void finished();

public slots:
    virtual void cancel();

protected slots:
    virtual void onAsyncCallFinished(QDBusPendingCallWatcher* watcher)// = 0?
    {
        Q_UNUSED (watcher);
    }

    void connectWatcher(QDBusPendingCallWatcher* watcher);

};

typedef QList<QPointer<AsyncOperation> > AsyncOperationList;

class SearchSortByExpression: public AsyncOperation
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

protected slots:
    virtual void onAsyncCallFinished(QDBusPendingCallWatcher* watcher);

private:
    QString mQuery;
    QString mSort;
    OrgGnomeEvolutionDataserverMailFolderInterface* const mFolderProxy;
    QDBusPendingCallWatcher* mPrepareSumWatcher;
    QDBusPendingCallWatcher* mSearchWatcher;
};

class GetMessageInfo: public AsyncOperation
{
    Q_OBJECT

public:
    explicit GetMessageInfo(OrgGnomeEvolutionDataserverMailFolderInterface *folderProxy, QObject* parent = 0) : AsyncOperation(parent), mFolderProxy(folderProxy), mCount(0)
    { Q_ASSERT (mFolderProxy); }

    virtual ~GetMessageInfo() {}

    void setMessageUids(const QStringList& uids);

public slots:
    void start(int count = 0);

signals:
    void result(const CamelMessageInfoVariant& info);

protected slots:
    virtual void onAsyncCallFinished(QDBusPendingCallWatcher* watcher);

private:
    QStringList mUids;
    OrgGnomeEvolutionDataserverMailFolderInterface* const mFolderProxy;
    int mCount;
};

class GetFolder: public AsyncOperation
{
    Q_OBJECT

public:
    explicit GetFolder(OrgGnomeEvolutionDataserverMailStoreInterface *storeProxy, OrgGnomeEvolutionDataserverMailStoreInterface *lstoreProxy = 0, QObject* parent = 0)
        :AsyncOperation(parent), mStoreProxy(storeProxy), mLstoreProxy(lstoreProxy) {}
    virtual ~GetFolder() {}

    void setFolderName(const QString& folderName);
    void setKnownFolders(const CamelFolderInfoArrayVariant& folders);

public slots:
    void start();

signals:
    void result(const QString& folderName, const QString& objectPath);

protected slots:
    virtual void onAsyncCallFinished(QDBusPendingCallWatcher *watcher);

private:
    QString mFolderName;
    OrgGnomeEvolutionDataserverMailStoreInterface *mStoreProxy;
    OrgGnomeEvolutionDataserverMailStoreInterface *mLstoreProxy;
    CamelFolderInfoArrayVariant mKnownFolders;
};

class GetAccount: public AsyncOperation
{
    Q_OBJECT

public:
    explicit GetAccount(OrgGnomeEvolutionDataserverMailSessionInterface* sessionProxy, QObject* parent = 0);
    virtual ~GetAccount() {}

    void setAccountName(const QString& account);

public slots:
    void start();
    virtual void cancel();

signals:
    void result(EAccount * account, const QString& storageObjectPath);

protected slots:
    virtual void onAsyncCallFinished(QDBusPendingCallWatcher *watcher);

private:
    QString mAccountName;
    OrgGnomeEvolutionDataserverMailSessionInterface* mSessionProxy;
    EAccount *mResultAccount;
};

class InitFolders: public AsyncOperation
{
    Q_OBJECT

public:
    explicit InitFolders(EAccount * account, OrgGnomeEvolutionDataserverMailStoreInterface* storeProxy, QObject* parent = 0);
    virtual ~InitFolders() {}
    void setLocalStore(OrgGnomeEvolutionDataserverMailStoreInterface *lStoreProxy)
    {
        mLStoreProxy = lStoreProxy;
    }

public slots:
    void start();
    virtual void cancel();

signals:
    void result(const CamelFolderInfoArrayVariant& folders);

protected slots:
    virtual void onAsyncCallFinished(QDBusPendingCallWatcher *watcher);

private:
    void createNewFolder(const QString& folderName);

private:
    OrgGnomeEvolutionDataserverMailStoreInterface *mStoreProxy;
    OrgGnomeEvolutionDataserverMailStoreInterface *mLStoreProxy;
    CamelFolderInfoArrayVariant mFolders;
    EAccount *mAccount;
    int mNewFoldersCount;
    QDBusPendingCallWatcher* mGetFoldersInfoWatcher;
    QDBusPendingCallWatcher* mGetOutboxFoldersInfoWatcher;
    QString mEmail;
    bool mPopAccount;
};

#endif // ASYNCCALLWRAPPER_H

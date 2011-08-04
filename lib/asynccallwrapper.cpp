#include "asynccallwrapper.h"
#include "e-gdbus-emailfolder-proxy.h"
#include "e-gdbus-emailstore-proxy.h"
#include "e-gdbus-emailsession-proxy.h"
// Camel
#define CAMEL_COMPILATION 1
#include <camel/camel-url.h>

#include <glib.h>
//#include <libedataserver/e-account-list.h>
#include <libedataserver/e-data-server-util.h>
#include <gconf/gconf-client.h>

/// AsyncOperation
AsyncOperation::AsyncOperation(QObject *parent)
    : QObject(parent)
{
}

void AsyncOperation::cancel()
{
    disconnect (this, SLOT(onAsyncCallFinished(QDBusPendingCallWatcher*)));
    emit finished();
}

void AsyncOperation::connectWatcher(QDBusPendingCallWatcher* watcher)
{
    Q_ASSERT (watcher);
    connect (watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onAsyncCallFinished(QDBusPendingCallWatcher*)));
    connect (watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher, SLOT(deleteLater()));
}

/// SearchSortByExpression
SearchSortByExpression::SearchSortByExpression(OrgGnomeEvolutionDataserverMailFolderInterface *folderProxy, QObject *parent)
    :AsyncOperation(parent), mFolderProxy(folderProxy), mPrepareSumWatcher(0), mSearchWatcher(0)
{
    Q_ASSERT (mFolderProxy);
}

void SearchSortByExpression::start()
{
    Q_ASSERT (mFolderProxy);
    if (mPrepareSumWatcher || mSearchWatcher) {
        qWarning() << Q_FUNC_INFO << "already started";
        return;
    }

    mPrepareSumWatcher = new QDBusPendingCallWatcher(mFolderProxy->prepareSummary(), this);
    connectWatcher (mPrepareSumWatcher);
}

void SearchSortByExpression::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT (watcher);
    qWarning() << Q_FUNC_INFO;
    if (watcher->isError()) {
        qWarning() << Q_FUNC_INFO << "dbus error occured";
        emit finished();
    } else {
        if (watcher == mPrepareSumWatcher) {
            mPrepareSumWatcher = 0;
            mSearchWatcher = new QDBusPendingCallWatcher(mFolderProxy->searchSortByExpression(mQuery, mSort, false), this);
            connectWatcher(mSearchWatcher);
        } else {
            Q_ASSERT (watcher == mSearchWatcher);
            mSearchWatcher = 0;
            QDBusPendingReply<QStringList> reply = *watcher;
            emit result(reply.value());
            emit finished();
        }
    }
}

///GetMessageInfo
void GetMessageInfo::setMessageUids(const QStringList& uids)
{
    mUids = uids;
}

void GetMessageInfo::start(int count)
{
    if (mCount > 0 ) {
        qWarning() << Q_FUNC_INFO << "already started";
        return;
    }

    mCount = (0 < count && count <= mUids.count()) ? count: mUids.count();

    if (mCount == 0) {
        emit finished();
        return;
    }

    for (int i = 0; i < mCount; ++i) {
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(mFolderProxy->getMessageInfo(mUids.at(i)), this);
        connectWatcher(watcher);
    }
}

void GetMessageInfo::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT (watcher);
    if (watcher->isError()) {
        qWarning() << Q_FUNC_INFO << "dbus error occured";
    } else {
        QDBusPendingReply <CamelMessageInfoVariant> reply = *watcher;
        emit result(reply.value());
    }
    if (--mCount <= 0) {
        emit finished();
    }
}


void GetFolder::setFolderName(const QString& folderName)
{
    mFolderName = folderName;
}

void GetFolder::setKnownFolders(const CamelFolderInfoArrayVariant &folders)
{
    mKnownFolders = folders;
}

void GetFolder::start()
{
    bool not_found = true;
    CamelFolderInfoVariant c_info;
    qDebug() << Q_FUNC_INFO << "Set folder key: " << mFolderName;
    foreach (const CamelFolderInfoVariant& info,  mKnownFolders) {
        if (mFolderName == info.uri) {
                c_info = info;
                not_found = false;
                break;
        }
    }

    if (not_found) {
        CamelURL *curl = camel_url_new (mFolderName.toUtf8(), NULL);
        c_info.full_name = QString(curl->path);
        qDebug() << "Path " + c_info.full_name;
        if (c_info.full_name.startsWith("/"))
                c_info.full_name.remove (0, 1);
        qDebug() << "newPath " + c_info.full_name;
        camel_url_free (curl);
    }

    QDBusPendingCallWatcher* watcher = 0;
    if (mFolderName.endsWith ("Outbox", Qt::CaseInsensitive)) {
        Q_ASSERT (mLstoreProxy);
        watcher = new QDBusPendingCallWatcher(mLstoreProxy->getFolder (QString(c_info.full_name)), this);
    } else if (mStoreProxy && mStoreProxy->isValid()) {
        watcher = new QDBusPendingCallWatcher(mStoreProxy->getFolder (QString(c_info.full_name)), this);
    }

    if (watcher) {
        connectWatcher(watcher);
    } else {
        emit finished();
    }

}

void GetFolder::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT (watcher);
    if (watcher->isError()) {
        qWarning() << Q_FUNC_INFO << "dbus error occured";
    } else {
        QDBusPendingReply <QDBusObjectPath> reply = *watcher;
        emit result(mFolderName, reply.value().path());
    }

    emit finished();
}

/// GetAccount

GetAccount::GetAccount(OrgGnomeEvolutionDataserverMailSessionInterface* sessionProxy, QObject* parent)
    :AsyncOperation(parent), mSessionProxy(sessionProxy), mResultAccount(0)
{
    Q_ASSERT (mSessionProxy);
}

void GetAccount::setAccountName(const QString& account)
{
    mAccountName = account;
}

void GetAccount::start()
{
    GConfClient *client = gconf_client_get_default ();
    EAccountList *account_list = e_account_list_new (client);
    g_object_unref (client);

    EIterator *iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
        EAccount *acc = (EAccount *) e_iterator_get (iter);
        if (mAccountName == acc->uid)
             mResultAccount = acc;
        e_iterator_next (iter);
    }

    g_object_ref (mResultAccount);
    g_object_unref (iter);
    g_object_unref (account_list);

    const char *url = e_account_get_string (mResultAccount, E_ACCOUNT_SOURCE_URL);

    if (mSessionProxy->isValid() && url && *url) {
        QDBusPendingCallWatcher* watcher = (strncmp (url, "pop:", 4) == 0)?
             new QDBusPendingCallWatcher(mSessionProxy->getLocalStore()):
             new QDBusPendingCallWatcher(mSessionProxy->getStore (QString(url)));
        connectWatcher(watcher);
    } else {
        qWarning() << Q_FUNC_INFO << "there is no valid session interface or url" << mAccountName << url;
        cancel();
    }
}

void GetAccount::cancel()
{
    mResultAccount = 0;
    AsyncOperation::cancel();
}

void GetAccount::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT (watcher);
    if (watcher->isError()) {
        qWarning() << Q_FUNC_INFO << "dbus error occured";
    } else {
        QDBusPendingReply <QDBusObjectPath> reply = *watcher;
        emit result(mResultAccount, reply.value().path());
    }

    emit finished();
}

/// InitFolders
InitFolders::InitFolders(EAccount * account, OrgGnomeEvolutionDataserverMailStoreInterface* storeProxy, QObject* parent)
    :AsyncOperation(parent), mStoreProxy(storeProxy), mLStoreProxy(0), mAccount(account),
      mNewFoldersCount(0), mGetFoldersInfoWatcher(0), mGetOutboxFoldersInfoWatcher(0), mPopAccount(false)
{
    Q_ASSERT (mAccount);
    Q_ASSERT (mStoreProxy);
}

void InitFolders::start()
{
   // cancel();
    const char *url = e_account_get_string (mAccount, E_ACCOUNT_SOURCE_URL);
    if (url && *url) {
        mPopAccount = (strncmp (url, "pop:", 4) == 0);
        if (mPopAccount) {
            mEmail = QString(e_account_get_string(mAccount, E_ACCOUNT_ID_ADDRESS));
        }

        if (mStoreProxy->isValid()) {
            mGetFoldersInfoWatcher = new QDBusPendingCallWatcher(mStoreProxy->getFolderInfo (mEmail,
                                                                    CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED));
            connectWatcher(mGetFoldersInfoWatcher);

        } else {
            qWarning() << Q_FUNC_INFO <<  "no valid storage";
            emit finished();
        }

    }
}

void InitFolders::cancel()
{
    mFolders.clear();
    mEmail = QString();
    mGetFoldersInfoWatcher = mGetOutboxFoldersInfoWatcher = 0;
    mNewFoldersCount = 0;
    mPopAccount = false;
    AsyncOperation::cancel();
}

void InitFolders::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT (watcher);
    if (watcher == mGetFoldersInfoWatcher) {

        if (watcher->isError()) {
            qWarning() << Q_FUNC_INFO << "dbus error. terminating";
            cancel();
        }
        QDBusPendingReply<CamelFolderInfoArrayVariant> reply = *watcher;
        CamelFolderInfoArrayVariant list = reply.value();

        if (!list.isEmpty())
            list.removeLast();

        mFolders.append (list);
        if (mFolders.isEmpty() && mPopAccount) {
            /* Create base first*/
            createNewFolder(QString("%1/Inbox").arg(mEmail));
            createNewFolder(QString("%1/Drafts").arg(mEmail));
            createNewFolder(QString("%1/Sent").arg(mEmail));
        } else if (mLStoreProxy) {
            // outbox folders need fetching
            mGetOutboxFoldersInfoWatcher = new QDBusPendingCallWatcher(mLStoreProxy->getFolderInfo ("Outbox", CAMEL_STORE_FOLDER_INFO_FAST));
            connectWatcher(mGetOutboxFoldersInfoWatcher);
        } else {
            emit result(mFolders);
            emit finished();
        }

    } else if (watcher == mGetOutboxFoldersInfoWatcher) {

        if (watcher->isError()) {
            qWarning() << Q_FUNC_INFO << "dbus error while outbox retrieval. terminating";
            cancel();
        } else {
            QDBusPendingReply<CamelFolderInfoArrayVariant> reply = *watcher;
            CamelFolderInfoArrayVariant outboxlist = reply.value();
            if (!outboxlist.isEmpty())
                outboxlist.removeLast();

            mFolders.append (outboxlist);
            qDebug() << Q_FUNC_INFO <<  "RESULT" << mFolders.count();

            emit result(mFolders);
        }

        emit finished();

    } else if (--mNewFoldersCount <= 0) {
        // means that pop folders are created, let's fetch them
        Q_ASSERT (mPopAccount);
        mGetFoldersInfoWatcher = new QDBusPendingCallWatcher(mStoreProxy->getFolderInfo (mEmail,
                                        CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED));
        connectWatcher(mGetFoldersInfoWatcher);
    }

}

void InitFolders::createNewFolder(const QString& folderName)
{
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(mStoreProxy->createFolder ("", folderName));
    connectWatcher(watcher);
    ++mNewFoldersCount;
}

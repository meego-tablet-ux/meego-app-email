#include "asynccallwrapper.h"
#include "e-gdbus-emailfolder-proxy.h"

/// SearchSortByExpression
SearchSortByExpression::SearchSortByExpression(OrgGnomeEvolutionDataserverMailFolderInterface *folderProxy, QObject *parent)
    :QObject(parent), mFolderProxy(folderProxy), mPrepareSumWatcher(0), mSearchWatcher(0)
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
    connect (mPrepareSumWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onAsyncCallFinished(QDBusPendingCallWatcher*)));
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
            connect (mSearchWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onAsyncCallFinished(QDBusPendingCallWatcher*)));
        } else {
            Q_ASSERT (watcher == mSearchWatcher);
            mSearchWatcher = 0;
            QDBusPendingReply<QStringList> reply = *watcher;
            emit result(reply.value());
            emit finished();
        }
    }
    watcher->deleteLater();
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
        connect (watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onAsyncCallFinished(QDBusPendingCallWatcher*)));
    }
}

void GetMessageInfo::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    qWarning() << Q_FUNC_INFO;
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
    watcher->deleteLater();
}

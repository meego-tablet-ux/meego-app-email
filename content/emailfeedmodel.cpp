/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "emailfeedmodel.h"

#include <QDebug>

#include <QDateTime>
#include <QStringList>
#include <QDesktopServices>

#include <actions.h>

#include <emailmessagelistmodel.h>
#include <folderlistmodel.h>

const int FetchBatch = 10;

// TODO: firm up this decision and move to a more central place
const int BodyPreviewSize = 160;  // big enough for an SMS, larger than a tweet

EmailFeedModel::EmailFeedModel(QVariant account, QObject *parent):
        McaFeedModel(parent)
{
    // TODO: pass the account id when Carl makes that available

    FolderListModel *folders = new FolderListModel(this);
    folders->setAccountKey(account);

    m_source = new EmailMessageListModel(this);
    m_source->setAccountKey(account);
    m_source->setFolderKey(folders->inboxFolderId());
    m_actions = new McaActions;

    delete folders;

    connect(m_source, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
    connect(m_source, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
    connect(m_source, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
    connect(m_source, SIGNAL(modelReset()),
            this, SLOT(resetModel()));
    connect(m_actions, SIGNAL(standardAction(QString,QString)),
            this, SLOT(performAction(QString,QString)));
    if(canFetchMore(QModelIndex())) fetchMore(QModelIndex());
}

EmailFeedModel::~EmailFeedModel()
{
    delete m_source;
    delete m_actions;
    int count = m_messages.count();
    for (int i = 0; i < count; i++)
    {
        delete m_messages.at(0);
        m_messages.removeAt(0);
    }
}

//
// public member functions
//
void EmailFeedModel::setSearchText(const QString &text)
{
    qDebug () << "Set Search: " << text;
    if (text.isEmpty() || m_searchText!=text) {
        m_source->setSearch(text);
        m_searchText=text;
    }
}

int EmailFeedModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_messages.count();
}

QVariant EmailFeedModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row >= m_messages.count())
        return QVariant();

    EmailMessage *message = m_messages.at(row);

    switch (role) {
    case RequiredTypeRole:
        return "content";

    case RequiredUniqueIdRole:
        return message->id;

    case RequiredTimestampRole:
        return message->timestamp;

    case GenericTitleRole:
        return message->contact;

    case GenericContentRole:
        return message->preview;

    case CommonActionsRole:
        return QVariant::fromValue<McaActions*>(m_actions);

    default:
        return QVariant();
    }

    return QVariant();
}

bool EmailFeedModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return (m_messages.count() < m_source->rowCount()) || m_source->stillMoreMessages();
}

void EmailFeedModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    if(isSearchHalted())
        return;
    int first = m_messages.count();
    int count = m_source->rowCount() - first;

    if (count > FetchBatch)
        count = FetchBatch;
    if (count > 0)
        sourceRowsInserted(QModelIndex(), first, first + count - 1);
    else if (m_source->stillMoreMessages())
        m_source->getMoreMessages();

    qDebug() << m_source->rowCount();
}


bool EmailFeedModel::removeRows ( int row, int count, const QModelIndex & parent )
{
    int first = row<0? 0:row;
    int last = ((first + count) > m_messages.count()? m_messages.count()-1: first+count-1);
    beginRemoveRows(parent, first, last);
    for (int i = first; i <= last; i++) {
        delete m_messages.at(first);
        m_messages.removeAt(first);
    }
    endRemoveRows();
    return true;
}

//
// protected slots
//

void EmailFeedModel::sourceRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    beginInsertRows(QModelIndex(), first, last);
    copyRowsFromSource(first, last);
    endInsertRows();
}

void EmailFeedModel::sourceRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    removeRows(first,last-first+1, parent);
}

void EmailFeedModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    int first = topLeft.row();
    int last = bottomRight.row();
    for (int i = first; i <= last; i++)
        readRow(m_messages.at(i), i);
    emit dataChanged(index(first), index(last));
}

void EmailFeedModel::resetModel()
{
    // use remove instead of begine/end model reset because it seems upper layer
    // not handling modelReset well yet.
    removeRows(0, m_messages.count());

    QModelIndex p;
    if(canFetchMore(p))
        fetchMore(p);
}

void EmailFeedModel::performAction(QString action, QString uniqueid)
{
    qDebug() << "Action" << action << "called for email item" << uniqueid;
    if (action == "default") {
        QString executable("meego-qml-launcher");
        QStringList parameters;
        parameters << "--app" << "meego-app-email";
        parameters << "--opengl" << "--fullscreen";
        parameters << "--cmd" << "openReader";
        parameters << "--cdata" << uniqueid;
        QProcess::startDetached(executable, parameters);
    }
}

//
// protected member functions
//

void EmailFeedModel::copyRowsFromSource(int first, int last)
{
    for (int i = last; i >= first; i--) {
        EmailMessage *message = new EmailMessage;
        readRow(message, i);
        m_messages.insert(first, message);
    }
}

void EmailFeedModel::readRow(EmailMessage *message, int row)
{
    QModelIndex sourceIndex = m_source->index(row);

    // prefer contact name to email address
    message->contact = m_source->data(sourceIndex, EmailMessageListModel::MessageSenderDisplayNameRole).toString();
    if (message->contact.isEmpty())
        message->contact = m_source->data(sourceIndex, EmailMessageListModel::MessageSenderEmailAddressRole).toString();

    // TODO: handle HTML mail more intelligently somehow, for now body is empty for HTML
    message->preview = m_source->data(sourceIndex, EmailMessageListModel::MessageSubjectTextRole).toString();
    if (message->preview.isEmpty()) {
        // TODO: use Carl's body preview role when he adds that
        QString body = m_source->data(sourceIndex, EmailMessageListModel::MessageBodyTextRole).toString();
        message->preview = body.left(BodyPreviewSize);
    }

    // TODO: wireframe apparently has an icon, will need to look that up in QtContacts

    message->id = m_source->data(sourceIndex, EmailMessageListModel::MessageUuidRole).toString();
    message->timestamp = m_source->data(sourceIndex, EmailMessageListModel::MessageTimeStampRole).toDateTime();

}

/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef __emailfeedmodel_h
#define __emailfeedmodel_h

#include <QObject>
#include <QList>
#include <QDateTime>

#include <feedmodel.h>
#include <actions.h>

struct EmailMessage {
    QString contact;
    QString preview;
    QString id;
    QDateTime timestamp;
};

class EmailMessageListModel;

class EmailFeedModel: public McaFeedModel, public McaSearchableFeed
{
    Q_OBJECT

public:
    EmailFeedModel(QVariant account, QObject *parent = 0, bool isSearchFeed = false);
    ~EmailFeedModel();

    void setSearchText(const QString &text);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);
    virtual bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );

    bool hasSearchText() const;

protected slots:
    void sourceRowsInserted(const QModelIndex& parent, int first, int last);
    void sourceRowsRemoved(const QModelIndex& parent, int first, int last);
    void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void resetModel();
    void performAction(QString uniqueid, QString action);
    void onAccountSet();

protected:
    void copyRowsFromSource(int first, int last);
    void readRow(EmailMessage *message, int row);

private:
    EmailMessageListModel *m_source;
    QList<EmailMessage *> m_messages;
    QString m_searchText;
    McaActions *m_actions;
    bool m_isSearchFeed;
    QVariant m_InboxId;
};

#endif  // __emailfeedmodel_h

/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef EMAILAGENT_H
#define EMAILAGENT_H

#include <QDeclarativeItem>

#include <QProcess>
#include <QTimer>
#include <mgconfitem.h>


class EmailAgent : public QDeclarativeItem
{
    Q_OBJECT

public:
    static EmailAgent *instance();

    explicit EmailAgent (QDeclarativeItem *parent = 0);
    ~EmailAgent();


    Q_INVOKABLE QString getSignatureForAccount (QVariant vMailAccountId);
    Q_INVOKABLE bool confirmDeleteMail ();
    Q_INVOKABLE bool openAttachment(const QString& attachmentDisplayName);
    Q_INVOKABLE void openBrowser(const QString& url);
    Q_INVOKABLE QString getMessageBodyFromFile(const QString& bodyFilePath);


private:
    static EmailAgent *m_instance;

    MGConfItem *m_confirmDeleteMail;
};

#endif

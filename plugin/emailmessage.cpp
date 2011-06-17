/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QDeclarativeItem>
#include <QFileInfo>
#include "emailagent.h"
#include "emailmessage.h"

EmailMessage::EmailMessage (QDeclarativeItem *parent)
    : QDeclarativeItem(parent)
{
}

EmailMessage::~EmailMessage ()
{
}

void EmailMessage::setFrom (const QString &sender)
{
}

void EmailMessage::setTo (const QStringList &toList)
{
}

void EmailMessage::setCc (const QStringList &ccList)
{
}

void EmailMessage::setBcc(const QStringList &bccList)
{
}

void EmailMessage::setSubject (const QString &subject)
{
}

void EmailMessage::setBody (const QString &body)
{
}

void EmailMessage::setAttachments (const QStringList &uris)
{
    m_attachments = uris;
}

void EmailMessage::setPriority (EmailMessage::Priority priority)
{
}

void EmailMessage::send()
{
}

void EmailMessage::saveDraft()
{
}

void EmailMessage::processAttachments ()
{
}

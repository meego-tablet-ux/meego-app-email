/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QDebug>

#include <QtPlugin>

#include "emailplugin.h"
#include "emailservicemodel.h"
#include "emailfeedmodel.h"

EmailPlugin::EmailPlugin(QObject *parent): QObject(parent), McaFeedPlugin()
{
    qDebug("EmailPlugin constructor");
    m_serviceModel = new EmailServiceModel(this);
}

EmailPlugin::~EmailPlugin()
{
}

QAbstractItemModel *EmailPlugin::serviceModel()
{
    return m_serviceModel;
}

QAbstractItemModel *EmailPlugin::createFeedModel(const QString& service)
{
    qDebug() << "EmailPlugin::createFeedModel: " << service;
    return new EmailFeedModel(m_serviceModel->accountId(service), this);
}

McaSearchableFeed *EmailPlugin::createSearchModel(const QString& service,
                                             const QString& searchText)
{
    qDebug() << "EmailPlugin::createSearchModel: " << service << searchText;
    EmailFeedModel* pModel=new EmailFeedModel(m_serviceModel->accountId(service), this, true);
    pModel->setSearchText(searchText);
    return pModel;
}

Q_EXPORT_PLUGIN2(emailplugin, EmailPlugin)

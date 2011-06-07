/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "components.h"
#include "folderlistmodel.h"
#include "emailaccountlistmodel.h"
#include "emailmessagelistmodel.h"
#include "emailagent.h"
#include "emailmessage.h"
#include "emailaccountsettingsmodel.h"
#include "emailaccount.h"
#include "htmlfield.h"

void components::registerTypes(const char *uri)
{
    qmlRegisterType<FolderListModel>(uri, 0, 0, "FolderListModel");
    qmlRegisterType<EmailAccountListModel>(uri, 0, 0, "EmailAccountListModel");
    qmlRegisterType<EmailMessageListModel>(uri, 0, 0, "EmailMessageListModel");
    qmlRegisterType<EmailAgent>(uri, 0, 0, "EmailAgent");
    qmlRegisterType<EmailMessage>(uri, 0, 0, "EmailMessage");
    qmlRegisterType<EmailAccountSettingsModel>(uri, 0, 0, "EmailAccountSettingsModel");
    qmlRegisterType<EmailAccount>(uri, 0, 0, "EmailAccount");
    qmlRegisterType<HtmlField>(uri, 0, 0, "HtmlField");
}

Q_EXPORT_PLUGIN(components);

/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1
import MeeGo.Labs.Components 0.1 as Labs
import MeeGo.App.Email 0.1

FocusScope {
    id: composerViewContainer
    focus: true

    property alias composer: composer

    width: parent.width
    height: parent.height

    ListModel {
        id: toRecipients
    }

    ListModel {
        id: ccRecipients
    }

    ListModel {
        id: bccRecipients
    }

    ListModel {
        id: attachments
    }

    Composer {
        id: composer
        focus: true
        anchors.fill: parent

        toModel: toRecipients
        ccModel: ccRecipients
        bccModel: bccRecipients
        attachmentsModel: mailAttachmentModel
        accountsModel: mailAccountListModel
    }
}


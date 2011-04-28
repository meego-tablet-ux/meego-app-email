/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1

Item {
    id: composer

    property string quotedBody: "";

    property alias body: editPane.text;
    property alias toModel: header.toModel
    property alias ccModel: header.ccModel
    property alias bccModel: header.bccModel
    property alias attachmentsModel: header.attachmentsModel
    property alias accountsModel: header.accountsModel
    property alias priority: header.priority
    property alias subject: header.subject
    property alias fromEmail: header.fromEmail
    property alias header: header

    function completeEmailAddresses () {
        header.completeEmailAddresses ();
    }

    Column {
        anchors.fill: parent
        anchors.topMargin: 10
        spacing: 5

        EmailHeader {
            id: header

            width: parent.width
            x: 10
            z: 1000
        }

        Image {
            width: parent.width
            height: composer.height - header.height - parent.spacing
            source: "image://theme/email/bg_reademail_l"

            TextField {
                id: editPane
                font.pixelSize: theme_fontPixelSizeLarge
                text : {
                    var sig = emailAgent.getSignatureForAccount(scene.currentMailAccountId);
                    if (sig == "")
                        return composer.quotedBody;
                    else
                        return (composer.quotedBody + "\n-- \n" + sig + "\n");
                }

                anchors.fill: parent

                onFocusChanged: {
                    console.log ("Focus changed " + focus);
                }
            }
        }
    }
}

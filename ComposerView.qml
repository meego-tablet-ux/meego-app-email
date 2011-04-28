/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1
import MeeGo.App.Email 0.1

Item {
    id: composerViewContainer

    property alias composer: composer
    parent: composerPage.content
    width: scene.content.width
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

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: composerViewToolbar.top

        toModel: toRecipients
        ccModel: ccRecipients
        bccModel: bccRecipients
        attachmentsModel: mailAttachmentModel
        accountsModel: mailAccountListModel
    }

    BorderImage {
        id: composerViewToolbar

        width: parent.width
        anchors.bottom: parent.bottom
        source: "image://theme/navigationBar_l"

        ToolbarButton {
            id: sendButton

            anchors.left: parent.left
            anchors.top: parent.top

            iconName: "mail-send"

            onClicked: {
                var i;
                var message;

                console.log ("Send email");

                composer.completeEmailAddresses ();

                message = messageComponent.createObject (composer);
                message.setFrom (composer.fromEmail);


                var to = new Array ();
                for (i = 0; i < composer.toModel.count; i++) {
                    to[i] = composer.toModel.get (i).email;
                }
                message.setTo (to);

                var cc = new Array ();
                for (i = 0; i < composer.ccModel.count; i++) {
                    cc[i] = composer.ccModel.get (i).email;
                }
                message.setCc (cc);

                var bcc = new Array ();
                for (i = 0; i < composer.bccModel.count; i++) {
                    bcc[i] = composer.bccModel.get (i).email;
                }
                message.setBcc (bcc);

                var att = new Array ();
                for (i = 0; i < composer.attachmentsModel.count; i++) {
                    att[i] = composer.attachmentsModel.get (i).uri;
                }
                message.setAttachments (att);

                message.setSubject (composer.subject);
                message.setPriority (composer.priority);
                message.setBody (composer.body);

                //message.send ();
		mailFolderListModel.sendMessage(composer.fromEmail, to, cc, bcc, composer.subject, composer.body, att, composer.priority);

                scene.previousApplicationPage ();
            }
        }

        ToolbarDivider {
            id: division1
            anchors.left: sendButton.right
            height: parent.height
        }

        ToolbarButton {
            id: saveButton

            anchors.left: division1.right
            anchors.top: parent.top

            iconName: "document-save"

            onClicked: {

                var i;
                var message;

                composer.completeEmailAddresses ();

                message = messageComponent.createObject (composer);
                message.setFrom (composer.fromEmail);

                var to = new Array ();
                for (i = 0; i < composer.toModel.count; i++) {
                    to[i] = composer.toModel.get (i).email;
                }
                message.setTo (to);

                var cc = new Array ();
                for (i = 0; i < composer.ccModel.count; i++) {
                    cc[i] = composer.ccModel.get (i).email;
                }
                message.setCc (cc);

                var bcc = new Array ();
                for (i = 0; i < composer.bccModel.count; i++) {
                    bcc[i] = composer.bccModel.get (i).email;
                }
                message.setBcc (bcc);

                var att = new Array ();
                for (i = 0; i < composer.attachmentsModel.count; i++) {
                    att[i] = composer.attachmentsModel.get (i).uri;
                }
                message.setAttachments (att);

                message.setSubject (composer.subject);
                message.setPriority (composer.priority);
                message.setBody (composer.body);

                //message.saveDraft ();
		mailFolderListModel.saveDraft (composer.fromEmail, to, cc, bcc, composer.subject, composer.body, att, composer.priority);
                scene.previousApplicationPage ();
            }
        }

        ToolbarDivider {
            id: division2
            anchors.left: saveButton.right
            height: parent.height
        }

        ToolbarDivider {
            id: division3
            anchors.right: cancelButton.left
            height: parent.height
        }

        ToolbarButton {
            id: cancelButton

            anchors.right: parent.right
            anchors.top: parent.top

            iconName: "edit-delete"

            onClicked: {
                verifyCancel.show();
            }
        }
    }

    Component {
        id: messageComponent

        EmailMessage {
            id: emailMessage
        }
    }

    ModalDialog {
        id: verifyCancel
        acceptButtonText: qsTr ("Yes")
        cancelButtonText: qsTr ("Cancel")
        title: qsTr ("Discard Email")
        content: Text {
            anchors.leftMargin: 10
            text: qsTr ("Are you sure you want to discard this unsent email?")
        }

        onAccepted: { scene.previousApplicationPage () }
    }


}

/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1
import MeeGo.Labs.Components 0.1 as Labs
import MeeGo.App.Email 0.1

Column {
    id: header

    property alias subject: subjectEntry.text
    property int fromEmail: 0

    property alias toModel: toRecipients.model
    property alias ccModel: ccRecipients.model
    property alias bccModel: bccRecipients.model
    property alias attachmentsModel: attachmentBar.model
    property EmailAccountListModel accountsModel
    property variant emailAccountList: []
    property int priority: 0

    property bool showOthers: false

    focus: true

    spacing: 5

    function completeEmailAddresses () {
        toRecipients.complete ();
        ccRecipients.complete ();
        bccRecipients.complete ();
    }

    Connections {
        target: mailAccountListModel
        onAccountAdded: {
            emailAccountList = accountsModel.getAllEmailAddresses();
            if (scene.currentMailAccountIndex == -1)
            {
                scene.currentMailAccountIndex = 0;
                fromEmail = 0
                accountSelector.selectedIndex = 0;
            }
        }
    }

    // EmailAccountListModel doesn't seem to be a real ListModel
    // We need to convert it to one to set it in the DropDown
    onAccountsModelChanged: {
        emailAccountList = accountsModel.getAllEmailAddresses();
        fromEmail = scene.currentMailAccountIndex;
    }

    Row {
        width: parent.width
        spacing: 5
        height: 53
        z: 1000

        VerticalAligner {
            id: fromLabel
            text: qsTr ("From:")
        }

        DropDown {
            id: accountSelector
            width: parent.width - (ccToggle.width + fromLabel.width + 30)
            minWidth: 400
            model: emailAccountList
            height: 53
            title: emailAccountList[0];
            titleColor: "black"
            replaceDropDownTitle: true

            Component.onCompleted: {
                selectedIndex = 0;
            }
            onTriggered: {
                fromEmail = index;
            }
        }

        Image {
            id: ccToggle
            width: ccBccLabel.width + 20
            height: parent.height

            source: "image://theme/btn_blue_up"

            Text {
                id: ccBccLabel
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Cc/Bcc")
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    header.showOthers = !header.showOthers;
                }
            }
        }
    }

    Row {
        //: The "to" recipient label.
        property string toLabel: qsTr("To")

        width: parent.width

        spacing: 5

        // Expand to fill the height correctly
        height: toRecipients.height

        EmailRecipientEntry {
            id: toRecipients

            defaultText: parent.toLabel
            width: parent.width - toAddButton.width - 20 - spacing
        }

        AddRecipient {
            id: toAddButton
            label: parent.toLabel
            recipients: toRecipients
        }
    }

    Row {
        //: The Cc (carbon copy) label.
        property string ccLabel: qsTr("Cc")

        width: parent.width
        spacing: 5

        height: ccRecipients.height
        visible: showOthers

        EmailRecipientEntry {
            id: ccRecipients

            defaultText: parent.ccLabel
            width: parent.width - ccAddButton.width - 20 - spacing
        }

        AddRecipient {
            id: ccAddButton
            label: parent.ccLabel
            recipients: ccRecipients
        }
    }

    Row {
        //: The Bcc (blind carbon copy) label.
        property string bccLabel: qsTr("Bcc")

        width: parent.width
        spacing: 5

        height: bccRecipients.height
        visible: showOthers

        EmailRecipientEntry {
            id: bccRecipients

            defaultText: parent.bccLabel
            width: parent.width - bccAddButton.width - 20 - spacing
        }

        AddRecipient {
            id: bccAddButton
            label: parent.bccLabel
            recipients: bccRecipients
        }
    }

    Row {
        width: parent.width
        height: 53
        spacing: 5

        TextEntry {
            id: subjectEntry

            width: parent.width - attachmentButton.width - 10
            height: parent.height

            defaultText: qsTr ("Enter subject here")
        }

        BorderImage {
            id: attachmentButton
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/email/icns_export/icn_addattachment"
            border.top: 5
            border.bottom: 5
            border.left: 5
            border.right: 5

            Component {
                id: addAttachment

                Labs.ApplicationPage {
                    id: addAttachmentPage
                    //: Attach a file (e.g. music, video, photo) to the document being composed.
                    title: qsTr("Attach a file")

                    AddAttachmentView {
                        attachments: attachmentBar.model

                    }
                }
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    scene.addApplicationPage(addAttachment)
                }
            }
        }

    }

    AttachmentView {
        id: attachmentBar
        width: parent.width - 20
        height: 41
        opacity: (model.count > 0) ? 1 : 0
    }

}

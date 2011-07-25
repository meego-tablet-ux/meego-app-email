/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1
import MeeGo.App.Email 0.1
import QtWebKit 1.0
import Qt.labs.gestures 2.0

Item {
    id: container
    anchors.fill: parent
    
    property string progressBarText: ""
    property bool progressBarVisible: false
    property real progressBarPercentage: 0
    property string uri;
    property bool downloadInProgress: false
    property bool openFlag: false
    property string saveLabel: qsTr("Save")
    property string openLabel: qsTr("Open")
    property string musicLabel: qsTr("Music")
    property string videoLabel: qsTr("Video")
    property string pictureLabel: qsTr("Picture")

    // @todo Remove if these are no longer relevant.
    property string attachmentSavedLabel: qsTr("Attachment saved.")
    property string downloadingAttachmentLabel: qsTr("Downloading Attachment...")
    property string downloadingContentLabel: qsTr("Downloading Content...")

    // Placeholder strings for I18n purposes.

    //: Message displayed when downloading an attachment.  Arg 1 is the name of the attachment.
    property string savingAttachmentLabel: qsTr("Saving %1")

    //: Attachment has been saved message, where arg 1 is the name of the attachment.
    property string attachmentHasBeenSavedLabel: qsTr("%1 saved")

    // Show CCed recipients.
    property bool showCc: true

    // Do we have Cc recipients?
    //
    // @todo For some reason the mailCc array length is 1 even though it is really empty.  Why?
    property bool ccListAvailable: mailCc.length > 0 && !(mailCc.length == 1 && mailCc[0] == "")

    Connections {
        target: messageListModel
        onMessageDownloadCompleted: {
            window.mailHtmlBody = messageListModel.htmlBody(window.currentMessageIndex);
        }
    }

    function save(saveRestore)
    {
        //save the body of the mail
        //saveRestore.setValue("window.currentMessageIndex", window.currentMessageIndex)
        saveRestore.setValue("window.mailHtmlBody", window.mailHtmlBody)
        saveRestore.setValue("window.mailSender",   window.mailSender)
        saveRestore.setValue( "window.mailBody", window.mailBody )
        saveRestore.setValue("window.mailSubject", window.mailSubject )
    }

    function restore(saveRestore)
    {
        //TODO: implement me
        //window.currentMessageIndex= saveRestore.value("window.currentMessageIndex")
        window.mailHtmlBody = saveRestore.value("window.mailHtmlBody")
        window.mailSender   = saveRestore.value("window.mailSender")
        window.mailBody     = saveRestore.value("window.mailBody")
        window.mailSubject  = saveRestore.value("window.mailSubject")
    }

    TopItem { id: topItem }

    ModalDialog {
        id: unsupportedFileFormat
        showCancelButton: false
        showAcceptButton: true
        acceptButtonText: qsTr("Ok")
        title: qsTr ("Warning")
        content: Item {
            anchors.fill: parent
            anchors.margins: 10
            Text {
                text: qsTr("File format is not supported.");
                color: theme.fontColorNormal
                font.pixelSize: theme.fontPixelSizeLarge
                wrapMode: Text.Wrap
            }
        }

        onAccepted: {}
    } 

    ContextMenu {
        id: attachmentContextMenu
        property alias model: attachmentActionMenu.model
        content: ActionMenu {
            id: attachmentActionMenu
            onTriggered: {
                attachmentContextMenu.hide();
                if (index == 0)  // open attachment
                {
                    openFlag = true;
                    var status = messageListModel.openAttachment(window.currentMessageIndex, uri);
                    if (status == false)
                        unsupportedFileFormat.show();
                }
                else if (index == 1) // Save attachment
                {
                    openFlag = false;
                    messageListModel.saveAttachment(window.currentMessageIndex, uri);
                }
            }

            /*  TODO:  need to add the download progress dialog back???
            Connections {
                target: emailAgent
                onAttachmentDownloadStarted: {
                    downloadInProgress = true;
                }
                onAttachmentDownloadCompleted: {
                    downloadInProgress = false;
                    if (openFlag == true)
                    {
                        var status = emailAgent.openAttachment(uri);
                        if (status == false)
                        {
                            unsupportedFileFormat.show();
                        }
                    }
                }
            } */
        }
    }  // end of attachmentContextMenu

    Rectangle {
        id: fromRect
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right:  ccToggle.visible ? ccToggle.left : parent.right
        anchors.rightMargin: ccToggle.visible ? 5 : 0
        height: 43
        Image {
            anchors.fill: parent
            fillMode: Image.Tile
            source: "image://theme/email/bg_email details_l"
        }
        Row {
            spacing: 5
            height: 43
            anchors.left: parent.left
            anchors.leftMargin: 3
            anchors.topMargin: 1
            Text {
                width: subjectLabel.width
                font.pixelSize: theme.fontPixelSizeMedium
                text: qsTr("From:")
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
            }
            EmailAddress {
                anchors.verticalCenter: parent.verticalCenter
                added: false
                emailAddress: window.mailSender
            }
        }
    }

    Button {
        id: ccToggle

        anchors.top:    fromRect.top
        anchors.bottom: fromRect.bottom
        anchors.right:  parent.right

        bgSourceUp: "image://theme/btn_blue_up"
        bgSourceDn: "image://theme/btn_blue_dn"

        minWidth: 60
        visible: ccListAvailable

        //: Label for CC recipient view toggle button.
        text: qsTr("Cc")

        onClicked: showCc = !showCc
    }

    Column {
        id: recipients

        width: parent.width
        anchors.top:  fromRect.bottom
        anchors.topMargin: 2

        spacing: 2

        RecipientViewRectangle {
            id: toRect
            height: fromRect.height

            recipients: mailRecipients
            //: "To" e-mail recipients label

            recipientLabel: qsTr("To:")
        }

        RecipientViewRectangle {
            id: ccRect

            visible: showCc && ccListAvailable

            recipients: mailCc

            //: "Cc" (carbon copy) recipients label
            recipientLabel: qsTr("Cc:")

            height: fromRect.height
        }

        // No need for Bcc recipients rectangle.  Nothing to show!
    }

    Rectangle {
        id: subjectRect
        anchors.top: recipients.bottom
        anchors.left: parent.left
        width: parent.width
        anchors.topMargin: 1
        clip: true
        height: 43
        Image {
            anchors.fill: parent
            fillMode: Image.Tile
	    source: "image://theme/email/bg_email details_l"
        }
        Row {
            spacing: 5
            height: 43
            anchors.left: parent.left
            anchors.leftMargin: 3
            Text {
                id: subjectLabel
                font.pixelSize: theme.fontPixelSizeMedium
                text: qsTr("Subject:")
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                width: subjectRect.width - subjectLabel.width - 10
                font.pixelSize: theme.fontPixelSizeNormal
                text: window.mailSubject
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
            }
        }
    }

    Rectangle {
        id: attachmentRect
        anchors.top: subjectRect.bottom
        anchors.topMargin: 1
        anchors.left: parent.left
        anchors.right: parent.right
        width: parent.width
        height: 41
        opacity: (window.numberOfMailAttachments > 0) ? 1 : 0
        AttachmentView {
            height: parent.height
            width: parent.width
            model: mailAttachmentModel

            onAttachmentSelected: {
                container.uri = uri;
                attachmentContextMenu.model = [openLabel, saveLabel];
                attachmentContextMenu.setPosition(mX, mY);
                attachmentContextMenu.show();
            }
        }
    }

    Rectangle {
        id: bodyTextArea
        anchors.top: (window.numberOfMailAttachments > 0) ? attachmentRect.bottom : subjectRect.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        border.width: 1
        border.color: "black"
        color: "white"


        HtmlView {
            id: htmlViewer
//            editable: false
            readOnly: true
            html: (window.mailHtmlBody == "") ? window.mailBody : window.mailHtmlBody;
            anchors.fill: parent
            anchors.topMargin: 2
            contentsScale: 1
            clip: true

            font.pixelSize: theme.fontPixelSizeLarge

            onLinkClicked: {
                emailAgent.openBrowser(url);
            }

            onLoadStarted: {
                progressBarText = downloadingContentLabel;
                progressBarVisible = true;
            }

            onLoadFinished: {
                progressBarVisible = false;
            }

            onLoadProgress: {
                progressBarPercentage=progress;
            }
        }
    }
}

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

    Connections {
        target: messageListModel
        onMessageDownloadCompleted: {
            window.mailHtmlBody = messageListModel.htmlBody(window.currentMessageIndex);
        }
    }

    function save(saveRestore)
    {
        //TODO: implement me
    }

    function restore(saveRestore)
    {
        //TODO: implement me
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
        width: parent.width
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

    Rectangle {
        id: toRect
        anchors.top: fromRect.bottom
        anchors.topMargin: 1
        anchors.left: parent.left
        width: parent.width
        height: 43
        Image {
            anchors.fill: parent
            fillMode: Image.Tile
            source: "image://theme/email/bg_email details_l"
        }
        Row {
            spacing: 5
            height: 43
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: 3
            Text {
                width: subjectLabel.width
                id: toLabel
                font.pixelSize: theme.fontPixelSizeMedium
                text: qsTr("To:")
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
            }
            EmailAddress {
                //FIX ME: There is more then one mail Recipient
                anchors.verticalCenter: parent.verticalCenter
                emailAddress: mailRecipients[0]
            }
        }
    }

    Rectangle {
        id: subjectRect
        anchors.top: toRect.bottom
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
        Flickable {
            id: flick
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 2
            width: parent.width
            height: parent.height

            property variant centerPoint

            contentWidth: {
                if (window.mailHtmlBody == "") 
                    return edit.paintedWidth;
                else
                    return htmlViewer.width;
            }
            contentHeight:  {
                if (window.mailHtmlBody == "") 
                    return edit.paintedHeight;
                else
                    return htmlViewer.height;
            }
            clip: true
         
            function ensureVisible(r)
            {
                if (contentX >= r.x)
                    contentX = r.x;
                else if (contentX+width <= r.x+r.width)
                    contentX = r.x+r.width-width;
                if (contentY >= r.y)
                    contentY = r.y;
                else if (contentY+height <= r.y+r.height)
                    contentY = r.y+r.height-height;
            }

            GestureArea {
                id: webGestureArea
                anchors.fill: parent

                Pinch {
                    id: webpinch
                    property real startScale: 1.0;
                    property real minScale: 0.5;
                    property real maxScale: 5.0;

                    onStarted: {

                        flick.interactive = false;
                        flick.centerPoint = window.mapToItem(flick, gesture.centerPoint.x, gesture.centerPoint.y);
                        startScale = htmlViewer.contentsScale;
                        htmlViewer.startZooming();
                    }

                    onUpdated: {
                        var cw = flick.contentWidth;
                        var ch = flick.contentHeight;

                        if (window.mailHtmlBody == "") {
                            var newPixelSize = edit.font.pixelSize * gesture.scaleFactor;
                            edit.font.pixelSize = Math.max(theme.fontPixelSizeLarge, Math.min(newPixelSize, theme.fontPixelSizeLargest3));
                        } else {
                            htmlViewer.contentsScale = Math.max(minScale, Math.min(startScale * gesture.totalScaleFactor, maxScale));
                        }

                        flick.contentX = (flick.centerPoint.x + flick.contentX) / cw * flick.contentWidth - flick.centerPoint.x;
                        flick.contentY = (flick.centerPoint.y + flick.contentY) / ch * flick.contentHeight - flick.centerPoint.y;

                    }


                    onFinished: {
                        htmlViewer.stopZooming();
                        flick.interactive = true;
                    }
                }
            }

            HtmlField {
                id: htmlViewer
                editable: false
                html: window.mailHtmlBody
                transformOrigin: Item.TopLeft
                anchors.left: parent.left
                anchors.topMargin: 2
                contentsScale: 1
                focus: true
                clip: true
                font.pixelSize: theme.fontPixelSizeLarge
                visible: (window.mailHtmlBody != "")
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

            TextEdit {
                id: edit
                anchors.left: parent.left
                anchors.leftMargin: 5
                width: flick.width
                height: flick.height
                focus: true
                wrapMode: TextEdit.Wrap
                //textFormat: TextEdit.RichText
                font.pixelSize: theme.fontPixelSizeLarge
                readOnly: true
                onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)
                text: window.mailBody
                visible:  (window.mailHtmlBody == "")
            }

        }
    }


}

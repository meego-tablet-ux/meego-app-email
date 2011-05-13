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
    id: folderListContainer
    width: scene.width
    parent: folderListView.content
    anchors.fill: parent

    property string chooseFolder: qsTr("Choose folder:")
    property string createNewFolder: qsTr("Create new folder")
    property string renameFolder: qsTr("Rename folder")
    property string deleteFolder: qsTr("Delete folder")
    property string attachments: qsTr("Attachments")
    property bool gettingMoreMessages: false
    property bool inSelectMode: false
    property int numOfSelectedMessages: 0

    Component.onCompleted: { 
        scene.folderListViewClickCount = 0;
        gettingMoreMessages = false;
    }

    Connections {
        target: messageListModel
        onMessageRetrievalCompleted: {
            gettingMoreMessages = false;
        }
    }

    ListModel {
        id: toModel
    }

    ListModel {
        id: ccModel
    }

    ListModel {
        id: attachmentsModel
    }

    function setMessageDetails (composer, messageID, replyToAll) {
        var dateline = qsTr ("On %1 %2 wrote:").arg(messageListModel.timeStamp (messageID)).arg(messageListModel.mailSender (messageID));

        composer.quotedBody = dateline + "\n" +  messageListModel.quotedBody (messageID); //i18n ok
        attachmentsModel.clear();
        composer.attachmentsModel = attachmentsModel;
        toModel.clear();
        toModel.append({"name": "", "email": messageListModel.mailSender(messageID)});
        composer.toModel = toModel;

        
        if (replyToAll == true)
        {
            ccModel.clear();
            var recipients = new Array();
            recipients = messageListModel.recipients(messageID);
            var idx;
            for (idx = 0; idx < recipients.length; idx++)
                ccModel.append({"name": "", "email": recipients[idx]});
            composer.ccModel = ccModel;
        }
        // "Re:" is not supposed to be translated as per RFC 2822 section 3.6.5
        // Internet Message Format - http://www.faqs.org/rfcs/rfc2822.html
        //
        // "If this is done, only one instance of the literal string
        // "Re: " ought to be used since use of other strings or more
        // than one instance can lead to undesirable consequences."
        // Also see: http://www.chemie.fu-berlin.de/outerspace/netnews/son-of-1036.html#5.4
        // FIXME: Also need to only add Re: if it isn't already in the subject
        // to prevent "Re: Re: Re: Re: " subjects.
        composer.subject = "Re: " + messageListModel.subject (messageID);  //i18n ok
    }

    ModalDialog {
        id: verifyDelete
        acceptButtonText: qsTr ("Yes")
        cancelButtonText: qsTr ("Cancel")
        title: qsTr ("Delete Email")
        content: Text {
            text: qsTr ("Are you sure you want to delete this email?")
        }

        onAccepted: { emailAgent.deleteMessage (scene.mailId) }
    }

    ContextMenu {
        id: contextMenu
        property alias model: contextActionMenu.model
        content: ActionMenu {
            id: contextActionMenu
            onTriggered: {

                contextMenu.hide();
                if (index == 0)  // Reply
                {
                    var newPage;
                    scene.addApplicationPage (composer);
                    newPage = scene.currentApplication;
                    setMessageDetails (newPage.composer, scene.currentMessageIndex, false);
                }
                else if (index == 1)   // Reply to all
                {
                    var newPage;
                    scene.addApplicationPage (composer);
                    newPage = scene.currentApplication;
                    setMessageDetails (newPage.composer, scene.currentMessageIndex, true);
                }
                else if (index == 2)   // Forward
                {
                    var newPage;
                    scene.addApplicationPage (composer);
                    newPage = scene.currentApplication;

                    newPage.composer.quotedBody = qsTr("-------- Forwarded Message --------") + messageListModel.quotedBody (scene.currentMessageIndex);
                    newPage.composer.subject = qsTr("[Fwd: %1]").arg(messageListModel.subject (scene.currentMessageIndex));
                    scene.mailAttachments = messageListModel.attachments(scene.currentMessageIndex);
		    messageListModel.saveAttachmentsInTemp (scene.currentMessageIndex);
                    mailAttachmentModel.init();
                    newPage.composer.attachmentsModel = mailAttachmentModel;
                }
                else if (index == 3)   // Delete
                {
                    if ( emailAgent.confirmDeleteMail())
                        verifyDelete.show();
                    else
                        messageListModel.deleteMessage (scene.mailId);
                }
                else if (index == 4)   // Mark as read/unread
                {
                    if (scene.mailReadFlag)
                    {
                        messageListModel.markMessageAsUnread (scene.mailId);
                        scene.mailReadFlag = 0;
                    }
                    else
                    {
                        messageListModel.markMessageAsRead (scene.mailId);
                        scene.mailReadFlag = 1;
                    }
                }
            }
        }
    }

    Item {
        id: emptyMailboxView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: folderListViewToolbar.top
        anchors.top: parent.top
        opacity: messageListView.count > 0 ? 0 : 1
        Text {
            id:confirmMsg
            text: qsTr ("There are no messages in this folder.")
            anchors.centerIn: emptyMailboxView
            color:theme_fontColorNormal
            font.pixelSize: theme_fontPixelSizeLarge
            elide: Text.ElideRight
        }
    }

    ListView {
        id: messageListView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: folderListViewToolbar.top
        width: parent.width
        clip: true

        opacity: count > 0 ? 1 : 0

        model: messageListModel

        footer: Rectangle {
            id: getMoreMessageRect
            height: 90
            width: parent.width
            visible: {
                var folderServerCount = mailFolderListModel.folderServerCount(scene.currentFolderId);
		messageListModel.setAccountKey (scene.currentMailAccountId);
		if (scene.currentFolderId)
			messageListModel.setFolderKey (scene.currentFolderId);

                if (messageListView.count < folderServerCount)
                    return true;
                else
                    return false;
            }
            Button {
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                height: 45
                width: 300
                text: {
                     if(gettingMoreMessages)
                         return  qsTr("Getting more messages")
                     else
                         return  qsTr("Get more messages")
                }
                onClicked: {
                    gettingMoreMessages = true;
                    messageListModel.getMoreMessages(scene.currentFolderId);
                }
            }
        }

        delegate: Rectangle {
            id: dinstance
            height: theme_listBackgroundPixelHeightTwo
            width: parent.width
            Image {
                id: itemBackground
                anchors.fill: parent
                source: {
                    if (inSelectMode)
                    {
                        return selected ? "image://theme/email/bg_unreademail_l" : "image://theme/email/bg_reademail_l";
                    }
                    else
                    {
                        return readStatus ? "image://theme/email/bg_reademail_l" : "image://theme/email/bg_unreademail_l";
                    }
                }
            }

            Image {
                id: readStatusIcon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source: "image://meegotheme/widgets/apps/email/email-unread"
                opacity: {
                    if (inSelectMode == true || readStatus == true)
                        return 0;
                    else
                        return 1;
                }
            }

            Image {
                id: selectIcon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source:"image://meegotheme/widgets/common/checkbox/checkbox-background"
                opacity: (inSelectMode == true && selected == 0) ? 1 : 0
            }

            Image {
                id: selectActiveIcon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source:"image://meegotheme/widgets/common/checkbox/checkbox-background-active"
                opacity: (inSelectMode == true && selected == 1) ? 1 : 0
            }

            property string msender
            msender: {
                var a;
                try
                {
                    a = sender ;
                }
                catch(err)
                {
                    a = "";
                }
                a[0] == undefined ? "":a[0];
            }
           
            Item {
                id: fromLine
                anchors.top: parent.top
                anchors.left: parent.left
                width: parent.width
                height: theme_listBackgroundPixelHeightTwo / 2

                Text {
                    id: senderText
                    anchors.left: parent.left
                    anchors.leftMargin: 50
                    width: (parent.width * 2) / 3
                    text: senderDisplayName != "" ? senderDisplayName : senderEmailAddress
                    font.bold: readStatus ? false : true
                    font.pixelSize: theme_fontPixelSizeNormal
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    elide: Text.ElideRight
                }
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    font.pixelSize: theme_fontPixelSizeSmall
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    text: fuzzy.getFuzzy(qDateTime);
                }
            }
            Item {
                id: subjectLine
                anchors.top: fromLine.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 50
                width: parent.width
                height: theme_listBackgroundPixelHeightTwo / 2

                Text {
                    id: subjectText
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    text: subject
                    width: (parent.width * 2) / 3
                    font.pixelSize: theme_fontPixelSizeNormal
                    elide: Text.ElideRight
                }
                Image {
                    id: attachmentLeft
                    anchors.right: attachmentMiddle.left
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    source: "image://theme/email/bg_attachment_left"
                    opacity: numberOfAttachments ? 1 : 0
                }
                Image {
                    id: attachmentMiddle
                    anchors.right: attachmentRight.left
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    width: numberOfAttachmentLabel.width + attachmentIcon.width + 1
                    source: "image://theme/email/bg_attachment_mid"
                    Text {
                        id: numberOfAttachmentLabel
                        anchors.verticalCenter: parent.verticalCenter
                        text: numberOfAttachments + " " // i18n ok
                        font.pixelSize: theme_fontPixelSizeNormal
                    }
                    opacity: numberOfAttachments ? 1 : 0
                }
                Image {
                    id: attachmentRight
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    anchors.rightMargin: 5
                    source: "image://theme/email/bg_attachment_right"
                    opacity: numberOfAttachments ? 1 : 0
                }
                Image {
                    id: attachmentIcon
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    height: attachmentMiddle.height
                    anchors.left: attachmentLeft.right
                    anchors.leftMargin: numberOfAttachmentLabel.width + 1
                    source: "image://theme/email/icn_paperclip"
                    z: 10000
                    opacity: numberOfAttachments ? 1 : 0
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (scene.folderListViewClickCount == 0)
                    {
                        if (inSelectMode)
                        {
                            if (selected)
                            {
                                messageListModel.deSelectMessage(index);
                                folderListContainer.numOfSelectedMessages = folderListContainer.numOfSelectedMessages - 1;
                            }
                            else
                            {
                                messageListModel.selectMessage(index);
                                folderListContainer.numOfSelectedMessages = folderListContainer.numOfSelectedMessages + 1;
                            }
                        }
                        else
                        {
                            scene.mailId = messageId;
                            scene.mailSubject = subject;
                            scene.mailSender = sender;
                            scene.mailTimeStamp = timeStamp;
                            scene.mailBody = body;
                            scene.mailQuotedBody = quotedBody;
                            scene.mailHtmlBody = htmlBody;
                            scene.mailAttachments = listOfAttachments;
                            scene.numberOfMailAttachments = numberOfAttachments;
                            scene.mailRecipients = recipients;
                            toListModel.init();
                            scene.mailCc = cc;
                            ccListModel.init();
                            scene.mailBcc = bcc;
                            bccListModel.init();
                            scene.currentMessageIndex = index;
                            mailAttachmentModel.init();
                            messageListModel.markMessageAsRead (messageId);
                            scene.mailReadFlag = true;
                            folderListView.addApplicationPage(reader);
                        }
                        scene.folderListViewClickCount = 0;
                        return;
                    }
                    scene.folderListViewClickCount++;
                }
                onPressAndHold: {
                    if (inSelectMode)
                        return;
                    scene.mailId = messageId;
                    scene.mailReadFlag = readStatus;
                    scene.currentMessageIndex = index;
                    var map = mapToItem(scene, mouseX, mouseY);
                    contextMenu.model = [qsTr("Reply"), qsTr("Reply to all"), qsTr("Forward"), qsTr("Delete"), 
                                         readStatus ? qsTr("Mark as unread") : qsTr("Mark as read")]
                    contextMenu.setPosition(map.x, map.y);
                    contextMenu.show();
                }
            }
        }
    }
    FolderListViewToolbar {
        id: folderListViewToolbar

        onEditModeBegin: {
            messageListModel.deSelectAllMessages();
            folderListContainer.inSelectMode = true;
            folderListContainer.numOfSelectedMessages = 0;
        }

        onEditModeEnd: {
            messageListModel.deSelectAllMessages();
            folderListContainer.inSelectMode = false;
        }
    }
}

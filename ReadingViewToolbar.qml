/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Labs.Components 0.1
import MeeGo.App.Email 0.1

Item {
    id: container

    height: navigationBar.height
    width: scene.content.width

    property int buttonWidth: (parent.width - 4) / 5

    ListModel {
        id: toModel
    }

    ListModel {
        id: ccModel
    }

    ListModel {
        id: attachmentsModel
    }

    Component {
        id: verifyDelete
        ModalDialog {
            leftButtonText: qsTr("Yes")
            rightButtonText: qsTr("Cancel")
            dialogTitle: qsTr ("Delete Email")
            contentLoader.sourceComponent: DialogText {
                text: qsTr ("Are you sure you want to delete this email?")
            }

            onDialogClicked: {
                dialogLoader.sourceComponent = undefined;
                if(button == 1)
                {
		    messageListModel.deleteMessage(scene.mailId);
                    scene.previousApplicationPage();
                }
            }
        }
    }

    function deleteMessage()
    {
	messageListModel.deleteMessage(scene.mailId);
        scene.previousApplicationPage();
    }

    function setMessageDetails (composer, messageID, replyToAll) {
        var dateline = qsTr ("On %1 %2 wrote:\n").arg(messageListModel.timeStamp (messageID)).arg(messageListModel.mailSender (messageID));

        composer.quotedBody = dateline + messageListModel.quotedBody (messageID);
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

    BorderImage {
        id: navigationBar
        width: parent.width
        source: "image://meegotheme/widgets/common/action-bar/action-bar-background"
    }
    Item  {
        anchors.left: parent.left
        width: parent.width

        Item {
            id: composeButton
            width: (scene.content.width - 4) / 5

            ToolbarButton {
                anchors.horizontalCenter: composeButton.horizontalCenter
                iconName: "mail-compose"
                onClicked: {
                    var newPage;
                    scene.addApplicationPage (composer);
                    newPage = scene.currentApplication;
                    attachmentsModel.clear();
                    newPage.composer.attachmentsModel = attachmentsModel;
                }
            }
        }
        Image {
            id: separator1
            anchors.left: composeButton.right
            source: "image://meegotheme/widgets/common/action-bar/action-bar-separator"
        }

        Item {
            id: replyButton
            anchors.left: separator1.right
            width: (scene.content.width - 4) / 5
            ToolbarButton {
                anchors.horizontalCenter: replyButton.horizontalCenter
                iconName: "mail-reply"
                onClicked: {
                    var newPage;

                    scene.addApplicationPage (composer);
                    newPage = scene.currentApplication;
                    setMessageDetails (newPage.composer, scene.currentMessageIndex, false);
		}
	    }
        }

        Image {
            id: separator2
            anchors.left: replyButton.right
            source: "image://meegotheme/widgets/common/action-bar/action-bar-separator"
        }

        Item {
            id:replyallButton
            anchors.left: separator2.right
            width: (scene.content.width - 4) / 5
            ToolbarButton {
                anchors.horizontalCenter: replyallButton.horizontalCenter
                iconName: "mail-reply-all"
                onClicked: {
                    var newPage;

                    scene.addApplicationPage (composer);
                newPage = scene.currentApplication;
                    setMessageDetails (newPage.composer, scene.currentMessageIndex, true);
                }
            }
        }

        Image {
            id: separator3
            anchors.left: replyallButton.right
            source: "image://meegotheme/widgets/common/action-bar/action-bar-separator"
        }

        Item {
            id:forwardButton
            anchors.left: separator3.right
            width: (scene.content.width - 4) / 5
            ToolbarButton {
                anchors.horizontalCenter: forwardButton.horizontalCenter
                iconName: "mail-forward"
                onClicked: {
                    var newPage;

                    scene.addApplicationPage (composer);
                    newPage = scene.currentApplication;

                    newPage.composer.quotedBody = qsTr("-------- Forwarded Message --------") + messageListModel.quotedBody (scene.currentMessageIndex);
                    newPage.composer.subject = qsTr("[Fwd: %1]").arg(messageListModel.subject (scene.currentMessageIndex));
                    newPage.composer.attachmentsModel = mailAttachmentModel;
                }
            }
        }

        Image {
            id: separator4
            anchors.left: forwardButton.right
            source: "image://meegotheme/widgets/common/action-bar/action-bar-separator"
        }

        Item {
            id: deleteButton
            anchors.left: separator4.right
            width: (scene.content.width - 4) / 5
            ToolbarButton {
                anchors.horizontalCenter: deleteButton.horizontalCenter
                iconName: "edit-delete"

                onClicked: {
                    if (emailAgent.confirmDeleteMail()) {
                        showModalDialog(verifyDelete);
                    } else {
                        deleteMessage();
                    }
                }
            }
        }
    }
}

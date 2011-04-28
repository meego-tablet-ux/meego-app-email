/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Labs.Components 0.1 as Labs
import MeeGo.Components 0.1
import MeeGo.App.Email 0.1

Item {
    id: container
    width: scene.width
    parent: accountListView.content
    anchors.fill: parent

    property int topicHeight: 58
    signal topicTriggered(int index)
    property alias currentTopic: listView.currentIndex
    property alias interactive: listView.interactive
    property alias model: listView.model

    Labs.ApplicationsModel {
        id: appModel
    }

    Component.onCompleted: {
        if (listView.count == 0)
        {
            var cmd = "/usr/bin/meego-qml-launcher --app meego-ux-settings --opengl --fullscreen --cmd showPage --cdata \"Email\"";  //i18n ok
            appModel.launch(cmd);
        }
        scene.currentMailAccountIndex = 0;
    }


    ListView {
        id: listView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: accountListViewToolbar.top
        width: parent.width
        clip: true
        model: mailAccountListModel
        spacing: 1

        onCurrentIndexChanged: container.topicTriggered(currentIndex)

        delegate: Rectangle {
            id: accountItem
            width: container.width
            height: theme_listBackgroundPixelHeightTwo

            property string accountDisplayName;
            accountDisplayName: {
                accountDisplayName = displayName;
                scene.currentAccountDisplayName = displayName;
                if (index == 0)
                    scene.currentMailAccountId = mailAccountId;
            }

            Image {
                anchors.fill: parent
                source: "image://theme/email/bg_email details_p"
            }

            property string accountImage;
            accountImage: {
                if (mailServer == "gmail")
                {
                    accountImage = "/usr/share/themes/"+ theme_name + "/icons/services/gmail.png";
                }
                else if (mailServer == "msn" || mailServer == "hotmail")
                {
                    accountImage = "/usr/share/themes/"+ theme_name + "/icons/services/msmail.png";
                }
                else if (mailServer == "facebook")
                {
                    accountImage = "/usr/share/themes/"+ theme_name + "/icons/services/facebook.png";
                }
                else if (mailServer == "yahoo")
                {
                    accountImage = "/usr/share/themes/"+ theme_name + "/icons/services/yahoo.png";
                }
                else if (mailServer == "aol")
                {
                    accountImage = "/usr/share/themes/"+ theme_name + "/icons/services/aim.png";
                }
                else
                {
                    accountImage = "/usr/share/themes/"+ theme_name + "/icons/services/generic.png";
                }
            }

            Image {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                source: accountImage
            }

            Text {
                id: accountName
                height: parent.height
                font.pixelSize: theme_fontPixelSizeLarge
                anchors.left: parent.left
                anchors.leftMargin: 100
                verticalAlignment: Text.AlignVCenter
                text: qsTr("%1 - %2").arg(emailAddress).arg(displayName)
            }

            Image {
                id: unreadImage
                anchors.right: goToFolderListIcon.left 
                anchors.rightMargin:10 
                anchors.verticalCenter: parent.verticalCenter
                width: 50
                fillMode: Image.Stretch
                source: "image://meegotheme/widgets/apps/email/accounts-unread"

                Text {
                    id: text
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    verticalAlignment: Text.AlignVCenter
                    text: unreadCount
                    font.pixelSize: theme_fontPixelSizeMedium
                    color: theme_fontColorNormal
                }
            }

            Image {
                id: goToFolderListIcon
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                source: "image://theme/arrow-right"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (scene.accountPageClickCount == 0)
                    {
                        listView.currentIndex = index;
                        scene.currentMailAccountId = mailAccountId;
                        scene.currentMailAccountIndex = index;
                        scene.currentAccountDisplayName = displayName;
                        messageListModel.setAccountKey (mailAccountId);
                        scene.folderListViewTitle = qsTr("%1 %2").arg(scene.currentAccountDisplayName).arg(mailFolderListModel.inboxFolderName());
                        scene.applicationPage = folderList;
                        scene.currentFolderId = mailFolderListModel.inboxFolderId();
                    }
                    scene.accountPageClickCount++;
                }
            }
        }
    }
    Item {
        id: accountListViewToolbar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: scene.width
        height: 120
        AccountViewToolbar {}
    }
}

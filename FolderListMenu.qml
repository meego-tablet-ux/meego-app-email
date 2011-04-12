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
import Qt.labs.gestures 2.0

Item {
    id: folderListMenu
    property bool scrollInFolderList: false
    height: {
        var realHeight = scene.width;
        if (scene.orientation == 1 || scene.orientation == 3)
        {
           realHeight = scene.height;
        }
        var maxHeight = 50 * (5 + mailFolderListModel.totalNumberOfFolders());
        if (maxHeight > (realHeight - 170))
        {
            scrollInFolderList = true;
            return (realHeight - 170);
        }
        else
            return maxHeight;
    }
    
    width: Math.max(sortTitle.width, goToFolderTitle.width) + 30
    Item {
        id: sort
        height: 50
        anchors.left: parent.left
        anchors.top: parent.top
        Text {
            id: sortTitle
            text: sortLabel
            font.bold: true
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color:theme_fontColorNormal
            font.pixelSize: theme_fontPixelSizeLarge
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
    }
    SortFilter {
        id: sortFilter
        anchors.top: sort.bottom
        anchors.left: parent.left
        width: parent.width
        height: 50 * 3
        topics: [
            topicDate,
            topicSender,
            topicSubject
        ]
        onTopicTriggered: {
            if (index == 0)
            {
                messageListModel.sortByDate(folderListView.dateSortKey);
                folderListView.dateSortKey = folderListView.dateSortKey ? 0 : 1;
                folderListView.senderSortKey = 1;
                folderListView.subjectSortKey = 1;
            }
            else if (index == 1)
            {
                messageListModel.sortBySender(folderListView.senderSortKey);
                folderListView.senderSortKey = folderListView.senderSortKey ? 0 : 1;
                folderListView.dateSortKey = 1;
                folderListView.subjectSortKey = 1;
            }
            else if (index == 2)
            {
                messageListModel.sortBySubject(folderListView.subjectSortKey);
                folderListView.subjectSortKey = folderListView.subjectSortKey ? 0 : 1;
                folderListView.dateSortKey = 1;
                folderListView.senderSortKey = 1;
            }
            folderListView.closeMenu()
        }
    }
    Image {
        id: sortDivider
        anchors.top: sortFilter.bottom
        width: parent.width
        source: "image://theme/email/divider_l"
    }
    Item {
        id: goToFolder
        height: 50
        anchors.left: parent.left
        anchors.top: sortDivider.bottom
        Text {
            id: goToFolderTitle
            text: scene.goToFolderLabel
            font.bold: true
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color:theme_fontColorNormal
            font.pixelSize: theme_fontPixelSizeLarge
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
    }
    ListView {
        id: listView
        anchors.left: parent.left
        anchors.top: goToFolder.bottom
        width: folderListMenu.width
        anchors.bottom: folderListMenu.bottom
        spacing: 1
        interactive: folderListMenu.scrollInFolderList
        clip: true
       
        model: mailFolderListModel

        delegate: Item {
            id: folderItem
            width: folderListMenu.width
            height: 50

            Image {
                width: folderListMenu.width
                source: "image://theme/email/divider_l"
            }

            Text {
                id: folderLabel
                height: 50
                text:  folderName
                font.pixelSize: theme_fontPixelSizeLarge
                color:theme_fontColorNormal
                anchors.left: parent.left
                anchors.leftMargin: 15
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            Text {
                height: 50
                font.pixelSize: theme_fontPixelSizeLarge
                text: qsTr("(%1)").arg(folderUnreadCount)
                anchors.left: folderLabel.right
                anchors.leftMargin: 10
                color:theme_fontColorNormal
                verticalAlignment: Text.AlignVCenter
                opacity: folderUnreadCount ? 1 : 0
            }

            GestureArea {
                anchors.fill: parent
                Tap {
                    onFinished: {
                        scene.currentFolderId = folderId;
                        folderListView.title = qsTr("%1 %2").arg(currentAccountDisplayName).arg(folderName);
                        folderListView.closeMenu();
                        messageListModel.setFolderKey(folderId);
                    }
                }
            }
        }
    }
}

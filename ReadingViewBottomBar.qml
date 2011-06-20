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
    width: parent.width
    height: previousNextEmailRect.height + readingViewToolbar.height + (progressBar.visible ? progressBar.height : 0)

    property alias progressBarText: progressText.text
    property alias progressBarVisible: progressBar.visible
    property alias progressBarPercentage: progressBarPrivate.percentage
    BorderImage {
        id: progressBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: previousNextEmailRect.top
        visible: false
        height: theme.fontPixelSizeLarge + 4
        source: "image://theme/navigationBar_l"

        ProgressBar {
            id: progressBarPrivate
            anchors.left: parent.left
            anchors.right: progressText.left
            anchors.bottom: parent.bottom
            height: parent.height
            fontColor: "white"
            fontColorFilled: "white"
        }

        Text {
            id: progressText
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            horizontalAlignment: Text.AlignLeft
            verticalAlignment:Text.AlignVCenter
            font.pixelSize: theme.fontPixelSizeLarge
            color: theme.fontColorMediaHighlight
        }
    }

    Item {
        id: previousNextEmailRect
        anchors.bottom: readingViewToolbar.top
        anchors.left: parent.left
        anchors.right: parent.right
        width: parent.width
        height: previousEmailButton.height
        //color: "#0d0303"
    BorderImage {
        id: navigationBar
        width: parent.width
        source: "image://themedimage/widgets/common/action-bar/action-bar-background"
    }

        ToolbarButton  {
            id: previousEmailButton
            anchors.left: parent.left
            anchors.top: parent.top
            visible: window.currentMessageIndex > 0 ? true : false
            iconName: "mail-message-previous" 
            onClicked: {
                if (window.currentMessageIndex > 0)
                {
                    window.currentMessageIndex = window.currentMessageIndex - 1;
                    window.updateReadingView(window.currentMessageIndex);
                }
            }
        }

        ToolbarButton {
            id: nextEmailButton

            anchors.right: parent.right
            anchors.top: parent.top
            visible: (window.currentMessageIndex + 1) < messageListModel.messagesCount() ? true : false
            iconName: "mail-message-next" 

            onClicked: {
                if (window.currentMessageIndex < messageListModel.messagesCount())
                {
                    window.currentMessageIndex = window.currentMessageIndex + 1;
                    window.updateReadingView(window.currentMessageIndex);
                }
            }
        }
    } 
    ReadingViewToolbar {
        id: readingViewToolbar
        width: parent.width
        anchors.bottom: parent.bottom
    }
}
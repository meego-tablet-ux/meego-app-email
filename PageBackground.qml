/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0

Image {
    property alias contents: contentArea.children
    property alias toolbar: toolbarArea.children
    anchors.fill: parent
    source: "image://themedimage/widgets/common/backgrounds/global-background-texture"
    clip: true
    BorderImage {
        id: panel
        property bool isLandscape: (window.inLandscape || window.inInvertedLandscape)
        property int landscapeSideMargin: 8 + (window.width - window.height)/2
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.leftMargin: isLandscape ? landscapeSideMargin : 8
        anchors.rightMargin: isLandscape ? landscapeSideMargin : 8
        anchors.bottomMargin: 5
        source: "image://themedimage/widgets/apps/media/content-background"
        border.left:   8
        border.top:    8
        border.bottom: 8
        border.right:  8
        Item {
            id: contentArea
            anchors.fill: parent
            anchors.topMargin: 8
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            anchors.bottomMargin: toolbarArea.height - 5
        }
    }
    Item {
        id: toolbarArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: childrenRect.height
    }
}

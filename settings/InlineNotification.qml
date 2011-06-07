/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1

Rectangle {
    property alias text: message.text
    color: "#FFFFAA"

    Theme {
        id: theme
    }

    Text {
        id: message
        anchors.fill: parent
        anchors.margins: 10
        color: "#FF0000"
        font.pixelSize: theme.fontPixelSizeNormal
    }
}

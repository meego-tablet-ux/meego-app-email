/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1

Column {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.leftMargin: 90
    anchors.rightMargin: 90
    property alias label: label.text
    property alias text: textentry.text
    property alias inputMethodHints: textentry.inputMethodHints
    property alias enabled: textentry.enabled
    property alias errorText: inlineNotification.text

    // private
    property bool suppress: true
    property bool emailField: false
    property int emailType: 2 //default to gmail

    signal textChanged()

    function setText(text) {
        suppress = true;
        textentry.text = text;
        suppress = false;
    }

    Theme {
        id: theme
    }

    Text {
        id: label
        height: 30
        font.pixelSize: theme.fontPixelSizeLarge
        font.italic: true
        color: "grey"
    }
    TextEntry {
        id: textentry
        anchors.left: parent.left
        anchors.right: parent.right
        onTextChanged: {
            //This piece of code auto fills the domain address. Currently only for gmail and yahoo
            if(emailField && emailType>0 && (textentry.text.indexOf("@")==-1)) {
                if(emailType == 2) {
                    textentry.text += qsTr("@gmail.com");
                } else if(emailType == 3) {
                    textentry.text += qsTr("@yahoo.com");
                }
                textentry.cursorPosition = 1;
            }

            if (!suppress)
                root.textChanged();
        }
    }
    InlineNotification {
        id: inlineNotification
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        visible: text.length > 0
    }
}

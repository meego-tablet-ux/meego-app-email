/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    spacing: 1

    WelcomeButton {
        title: qsTr("AOL")
        icon: "/usr/share/themes/" + theme_name + "/icons/services/aim.png"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 4; // AOL
            emailAccount.description = qsTr("AOL");
            settingsPage.state = "RegisterScreen";
        }
    }
    WelcomeButton {
        title: qsTr("Gmail")
        icon: "/usr/share/themes/" + theme_name + "/icons/services/gmail.png"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 2; // Gmail
            emailAccount.description = qsTr("Gmail");
            settingsPage.state = "RegisterScreen";
        }
    }
    WelcomeButton {
        title: qsTr("Microsoft Live Hotmail")
        icon: "/usr/share/themes/" + theme_name + "/icons/services/msmail.png"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 5; // Microsoft Live
            emailAccount.description = qsTr("Microsoft Live Hotmail");
            settingsPage.state = "RegisterScreen";
        }
    }
    // disabled till we can get it working
    /*
    WelcomeButton {
        title: qsTr("Mobile Me")
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 1; // Mobile Me
            emailAccount.description = qsTr("MobileMe");
            settingsPage.state = "RegisterScreen";
        }
    }
    */
    WelcomeButton {
        title: qsTr("Yahoo!")
        icon: "/usr/share/themes/" + theme_name + "/icons/services/yahoo.png"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 3; // Yahoo
            emailAccount.description = qsTr("Yahoo!");
            settingsPage.state = "RegisterScreen";
        }
    }
    WelcomeButton {
        title: qsTr("Other")
        onClicked: {
            emailAccount.clear();
            emailAccount.recvSecurity = "0"; // None
            emailAccount.sendAuth = "0";     // None
            emailAccount.sendSecurity = "0"; // None
            settingsPage.state = "RegisterScreen";
        }
    }
}

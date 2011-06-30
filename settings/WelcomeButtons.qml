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
    anchors.left: parent.left
    anchors.right: parent.right

    spacing: 1

    // disabled till we get the license agreement from AOL
    /*
    WelcomeButton {
        title: qsTr("AOL")
        icon: "image://themedimage/icons/services/aim"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 4; // AOL
            emailAccount.description = qsTr("AOL");
            savePreset();
            settingsPage.state = "RegisterScreen";
        }
    }
    */
    WelcomeButton {
        title: qsTr("Gmail")
        icon: "image://themedimage/icons/services/gmail"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 2; // Gmail
            emailAccount.description = qsTr("Gmail");
            savePreset();
            settingsPage.state = "RegisterScreen";
        }
    }
    WelcomeButton {
        title: qsTr("Microsoft Live Hotmail")
        icon: "image://themedimage/icons/services/msmail"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 5; // Microsoft Live
            emailAccount.description = qsTr("Microsoft Live Hotmail");
            savePreset();
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
            savePreset();
            settingsPage.state = "RegisterScreen";
        }
    }
    */
    // disabled till we get the license agreement from yahoo
    /*
    WelcomeButton {
        title: qsTr("Yahoo!")
        icon: "image://themedimage/icons/services/yahoo"
        onClicked: {
            emailAccount.clear();
            emailAccount.preset = 3; // Yahoo
            emailAccount.description = qsTr("Yahoo!");
            savePreset();
            settingsPage.state = "RegisterScreen";
        }
    }*/
    WelcomeButton {
        title: qsTr("Other")
        icon: "image://themedimage/icons/services/generic"
        onClicked: {
            emailAccount.clear();
            emailAccount.description = qsTr("");
            emailAccount.recvSecurity = "0"; // None
            emailAccount.sendAuth = "0";     // None
            emailAccount.sendSecurity = "0"; // None
            saveZero();
            settingsPage.state = "RegisterScreen";
        }
    }

    SaveRestoreState {
        id: welcomeButtonsSave
        onSaveRequired: {
            sync();
        }
    }

    function savePreset() { //Called by other items' signal
        welcomeButtonsSave.setValue("email-account-preset",emailAccount.preset);
        welcomeButtonsSave.setValue("email-account-description",emailAccount.description);
        welcomeButtonsSave.sync();
    }

    function saveZero() { //Called by other items' signal
        welcomeButtonsSave.setValue("email-account-preset","-1");
        welcomeButtonsSave.setValue("email-account-recvSecurity",emailAccount.recvSecurity);
        welcomeButtonsSave.setValue("email-account-sendAuth",emailAccount.sendAuth);
        welcomeButtonsSave.setValue("email-account-sendSecurity",emailAccount.sendSecurity);
        welcomeButtonsSave.sync();
    }

}

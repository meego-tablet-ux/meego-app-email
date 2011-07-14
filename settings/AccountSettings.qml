/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1
import "settings.js" as Settings

Item {
    Theme {
        id: theme
    }

    height: content.height

    onHeightChanged: {
        settingsPage.height = height;
    }

    Column {
        id: content
        width: settingsPage.width - 20
        spacing: 2
        Subheader { text: qsTr("Accounts") }
        Repeater {
            model: accountSettingsModel
            delegate: AccountExpandobox {}
        }
        // setup new account button, mimics expandobox appearance
        Rectangle {
            color: "white"
            anchors.left: parent.left
            anchors.right: parent.right
            height: 77
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 100
                font.pixelSize: theme.fontPixelSizeLarge
                color: theme.fontColorNormal
                text: qsTr("Set up new account")
            }
            Image {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10
                source: "image://theme/settings/pulldown_arrow_dn"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: settingsPage.state = "WelcomeScreen"
            }
        }
        Subheader { text: qsTr("General Settings") }
        Expandobox {
            barContent: Component {
                Item {
                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        font.pixelSize: theme.fontPixelSizeLarge
                        elide: Text.ElideRight
                        color: theme.fontColorNormal
                        text: qsTr("Update:")
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        font.pixelSize: theme.fontPixelSizeMedium
                        color: "#2fa7d4"
                        text: Settings.textForInterval(updateInterval.selectedValue)
                    }
                }
            }
            RadioGroup {
                id: updateInterval
                selectedValue: accountSettingsModel.updateInterval()
                onSelectedValueChanged: accountSettingsModel.setUpdateInterval(selectedValue)
            }
            content: Component {
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: updateColumn.height
                    color: "#eaf6fb"
                    Column {
                        id: updateColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 5
                        anchors.rightMargin: 5
                        spacing: 2
                        IntervalOption { value: 5 }
                        IntervalOption { value: 15 }
                        IntervalOption { value: 30 }
                        IntervalOption { value: 60 }
                        IntervalOption { value: 0 }
                        Item { width: 1; height: 10; }
                    }
                }
            }
        }
        Expandobox {
            barContent: Component {
                Item {
                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        font.pixelSize: theme.fontPixelSizeLarge
                        elide: Text.ElideRight
                        color: theme.fontColorNormal
                        text: qsTr("Signature")
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        font.pixelSize: theme.fontPixelSizeMedium
                        color: "#2fa7d4"
                        elide: Text.ElideRight
                        //: %1 is the email signature, a block of text appended to the end of an e-mail message.
                        text: (accountSettingsModel.signature()=="")? qsTr("Enter your signature."): qsTr("\"%1\"").arg(accountSettingsModel.signature())
                    }
                }
            }
            content: Component {
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    color: "#eaf6fb"
                    height: 100
                    TextField {
                        id: signature
                        anchors.fill: parent
                        anchors.margins: 10
                        text: (accountSettingsModel.signature()=="")? qsTr("Enter your signature.") :
                                      accountSettingsModel.signature()
                        onTextChanged: accountSettingsModel.setSignature(text)
                    }
                }
            }
        }
        Expandobox {
            barContent: Component {
                Item {
                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        font.pixelSize: theme.fontPixelSizeLarge
                        color: theme.fontColorNormal
                        text: qsTr("Notifications")
                    }
                }
            }
            content: Component {
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: notificationsColumn.height
                    color: "#eaf6fb"
                    Column {
                        id: notificationsColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 5
                        anchors.rightMargin: 5
                        spacing: 2
                        NotificationSetting {
                            text: qsTr("New email notifications, which means that these notifications are a new feature not previously available on this device. It is left as an assignment for the sucker who bought this device to find out what notifications exactly they can expect to get, having selected this option. Use at your own risk!")
                            on: accountSettingsModel.newMailNotifications()
                            onOnChanged: accountSettingsModel.setNewMailNotifications(on)
                        }
                        NotificationSetting {
                            text: qsTr("Ask before deleting email")
                            on: accountSettingsModel.confirmDeleteMail()
                            onOnChanged: accountSettingsModel.setConfirmDeleteMail(on)
                        }
                        Item { width: 1; height: 10; }
                    }
                }
            }
        }
        //FIXME use standard action bar here
        Rectangle {
            id: buttonBar
            anchors.bottom: parent.bottom
            width: parent.width
            height: 70
            color: "grey"
            Button {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                height: 45
                anchors.margins: 10
                //color: "white"
                text: qsTr("Save changes")
                onClicked: {
                    accountSettingsModel.saveChanges();
                    // cdata will be set if called from email app
                    if (window.applicationData) {
                        settingsPage.returnToEmail();
                    } else {
                        changesSaved.show();
                    }
                }
            }
            Button {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 45
                anchors.margins: 10
                //color: "white"
                text: qsTr("Cancel")
                onClicked: {
                    verifyCancel.show();
                }
            }
        }
    }

    ModalMessageBox {
        id: verifyCancel
        acceptButtonText: qsTr ("Yes")
        cancelButtonText: qsTr ("No")
        title: qsTr ("Discard changes")
        text: qsTr ("You have made changes to your settings. Are you sure you want to cancel?")
        onAccepted: { settingsPage.returnToEmail(); }
    }
    ModalMessageBox {
        id: changesSaved
        acceptButtonText: qsTr ("OK")
        showCancelButton: false
        title: qsTr ("Changes saved")
        text: qsTr ("Your changes have been saved.")
    }


}

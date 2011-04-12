/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Labs.Components 0.1
import MeeGo.Settings 0.1
import "settings.js" as Settings

Item {
    property string message
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: "#eaf6fb"
    }
    Flickable {
        clip: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonBar.top
        contentWidth: content.width
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick
        Item {
            width: settingsPage.width
            Column {
                id: content
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                Item { width: 1; height: 20; }
                Text {
                    font.pixelSize: theme_fontPixelSizeLarge
                    font.weight: Font.Bold
                    text: message
                }
                ControlGroup {
                    title: qsTr("Receiving settings")
                    subtitle: qsTr("You may need to contact your email provider for these settings.")
                    children: [
                        Item { width: 1; height: 1; },   // spacer
                        DropDownControl {
                            label: qsTr("Server type")
                            dataList: Settings.serviceModel
                            selectedValue: Settings.serviceName(emailAccount.recvType)
                            onSelectionChanged: emailAccount.recvType = Settings.serviceCode(data)
                        },
                        TextControl {
                            id: recvServerField
                            label: qsTr("Server address")
                            Component.onCompleted: setText(emailAccount.recvServer)
                            inputMethodHints: Qt.ImhNoAutoUppercase
                            onTextChanged: emailAccount.recvServer = text
                        },
                        TextControl {
                            id: recvPortField
                            label: qsTr("Port")
                            Component.onCompleted: setText(emailAccount.recvPort)
                            inputMethodHints: Qt.ImhDigitsOnly
                            onTextChanged: emailAccount.recvPort = text
                        },
                        DropDownControl {
                            label: qsTr("Security")
                            dataList: Settings.encryptionModel
                            selectedValue: Settings.encryptionName(emailAccount.recvSecurity)
                            onSelectionChanged: emailAccount.recvSecurity = Settings.encryptionCode(data)
                        },
                        TextControl {
                            id: recvUsernameField
                            label: qsTr("Username")
                            Component.onCompleted: setText(emailAccount.recvUsername)
                            inputMethodHints: Qt.ImhNoAutoUppercase
                            onTextChanged: emailAccount.recvUsername = text
                        },
                        PasswordControl {
                            id: recvPasswordField
                            label: qsTr("Password")
                            Component.onCompleted: setText(emailAccount.recvPassword)
                            onTextChanged: emailAccount.recvPassword = text
                        },
                        Item { width: 1; height: 1; }   // spacer
                    ]
                }
                ControlGroup {
                    title: qsTr("Sending settings")
                    subtitle: qsTr("You may need to contact your email provider for these settings.")
                    children: [
                        Item { width: 1; height: 1; },   // spacer
                        TextControl {
                            id: sendServerField
                            label: qsTr("Server address")
                            Component.onCompleted: setText(emailAccount.sendServer)
                            inputMethodHints: Qt.ImhNoAutoUppercase
                            onTextChanged: emailAccount.sendServer = text
                        },
                        TextControl {
                            id: sendPortField
                            label: qsTr("Port")
                            Component.onCompleted: setText(emailAccount.sendPort)
                            inputMethodHints: Qt.ImhDigitsOnly
                            onTextChanged: emailAccount.sendPort = text
                        },
                        DropDownControl {
                            id: sendAuthField
                            label: qsTr("Authentication")
                            dataList: Settings.authenticationModel
                            selectedValue: Settings.authenticationName(emailAccount.sendAuth)
                            onSelectionChanged: emailAccount.sendAuth = Settings.authenticationCode(data)
                        },
                        DropDownControl {
                            label: qsTr("Security")
                            dataList: Settings.encryptionModel
                            selectedValue: Settings.encryptionName(emailAccount.sendSecurity)
                            onSelectionChanged: emailAccount.sendSecurity = Settings.encryptionCode(data)
                        },
                        TextControl {
                            id: sendUsernameField
                            label: qsTr("Username")
                            Component.onCompleted: setText(emailAccount.sendUsername)
                            inputMethodHints: Qt.ImhNoAutoUppercase
                            onTextChanged: emailAccount.sendUsername = text
                        },
                        PasswordControl {
                            id: sendPasswordField
                            label: qsTr("Password")
                            Component.onCompleted: setText(emailAccount.sendPassword)
                            onTextChanged: emailAccount.sendPassword = text
                        },
                        Item { width: 1; height: 1; }   // spacer
                    ]
                }
            }
        }
    }
    Component {
        id: verifyCancel
        ModalDialog {
            property variant settingsPage
            leftButtonText: qsTr ("Yes")
            rightButtonText: qsTr ("No")
            dialogTitle: qsTr ("Discard changes")
            contentLoader.sourceComponent: DialogText {
                text: qsTr ("You have made changes to your settings, are you sure you want to cancel?")
            }

            onDialogClicked: {
                dialogLoader.sourceComponent = undefined;
                if (button == 1) {
                    settingsPage.state = settingsPage.getHomescreen()
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
            width: 200
            height: 45
            anchors.margins: 10
            //color: "white"
            title: qsTr("Next")
            function validate() {
                var errors = 0;
                if (recvServerField.text.length === 0) {
                    recvServerField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    recvServerField.errorText = "";
                }
                if (recvPortField.text.length === 0) {
                    recvPortField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    recvPortField.errorText = "";
                }
                if (recvUsernameField.text.length === 0) {
                    recvUsernameField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    recvUsernameField.errorText = "";
                }
                if (recvPasswordField.text.length === 0) {
                    recvPasswordField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    recvPasswordField.errorText = "";
                }

                if (sendServerField.text.length === 0) {
                    sendServerField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    sendServerField.errorText = "";
                }
                if (sendPortField.text.length === 0) {
                    sendPortField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    sendPortField.errorText = "";
                }
                // FIXME: reading selectedValue doesn't work with the current DropDown implementation
                if (sendAuthField.selectedValue != Settings.authenticationModel[0] && sendUsernameField.text.length === 0) {
                    sendUsernameField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    sendUsernameField.errorText = "";
                }
                if (sendAuthField.selectedValue != Settings.authenticationModel[0] && sendPasswordField.text.length === 0) {
                    sendPasswordField.errorText = qsTr("This field is required");
                    errors++;
                } else {
                    sendPasswordField.errorText = "";
                }
                console.log(sendAuthField.selectedValue);
                return errors === 0;
            }
            onClicked: {
                if (validate())
                    settingsPage.state = "DetailsScreen";
            }
        }
        Button {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 200
            height: 45
            anchors.margins: 10
            //color: "white"
            title: qsTr("Cancel")
            onClicked: {
                showModalDialog(verifyCancel);
                dialogLoader.item.settingsPage = settingsPage;
            }
        }
    }
}

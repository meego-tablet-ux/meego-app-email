/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1
import MeeGo.Settings 0.1
import "settings.js" as Settings

Item {
    id: root
    property variant overlay: null

    Rectangle {
        anchors.fill: parent
        color: "#eaf6fb"
    }
    height: content.height

    onHeightChanged: {
        settingsPage.height = root.height;
    }


    Theme {

        id:theme
    }

    Column {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        width: settingsPage.width
        spacing: 10
        Text {
            font.weight: Font.Bold
            text: qsTr("Account details")
        }
        Text {
            text: qsTr("Account: %1").arg(emailAccount.description)
        }
        Text {
            text: qsTr("Name: %1").arg(emailAccount.name)
        }
        Text {
            text: qsTr("Email address: %1").arg(emailAccount.address)
        }
        Item {
            width: 1
            height: 20
        }
        Text {
            text: qsTr("Receiving:")
        }
        Text {
            id: textServerType
            text: qsTr("Server type: %1").arg(Settings.serviceName(emailAccount.recvType))
        }
        Text {
            id: textServerAddress
            text: qsTr("Server address: %1").arg(emailAccount.recvServer)
        }
        Text {
            id: textServerPort
            text: qsTr("Port: %1").arg(emailAccount.recvPort)
        }
        Text {
            id: textServerSecurity
            text: qsTr("Security: %1").arg(Settings.encryptionName(emailAccount.recvSecurity))
        }
        Text {
            id: textServerUsername
            text: qsTr("Username: %1").arg(emailAccount.recvUsername)
        }
        Item {
            width: 1
            height: 20
        }
        Text {
            text: qsTr("Sending:")
        }
        Text {
            id: textSendServerAddress
            text: qsTr("Server address: %1").arg(emailAccount.sendServer)
        }
        Text {
            id: textSendPort
            text: qsTr("Port: %1").arg(emailAccount.sendPort)
        }
        Text {
            id: textSendAuthentication
            text: qsTr("Authentication: %1").arg(Settings.authenticationName(emailAccount.sendAuth))
        }
        Text {
            id: textSendServerSecurity
            text: qsTr("Security: %1").arg(Settings.encryptionName(emailAccount.sendSecurity))
        }
        Text {
            id: textSendServerUsername
            text: qsTr("Username: %1").arg(emailAccount.sendUsername)
        }

        Row{
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            Button {
                height: 45
                text: qsTr("Cancel")
//                bgSourceUp: "image://theme/btn_grey_up"
//                bgSourceDn: "image://theme/btn_grey_dn"
                onClicked: {
                    verifyCancel.show();
                }
            }
            Button {
                height: 45
                text: qsTr("Next")
//                bgSourceUp: "image://theme/btn_blue_up"
//                bgSourceDn: "image://theme/btn_blue_dn"
                onClicked: {
                    emailAccount.save();
                    emailAccount.test();
                    spinner.show();
                }

                Connections {
                    target: emailAccount
                    onTestSucceeded: {
                        spinner.hide();
                        settingsPage.state = "ConfirmScreen";
                    }
                    onTestFailed: {
                        spinner.hide();
                        errorDialog.show();
                        emailAccount.remove();
                    }
                }
            }
        }
        Rectangle {
            z: 1
            color: "white"
            anchors.left: parent.left
            anchors.right: parent.right
            height: 77
            MouseArea {
                anchors.fill: parent
                onClicked: {  settingsPage.state = "ManualScreen";
                              loader.item.message = qsTr("Please fill in account details:");
                            }
            }
            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 10
                font.pixelSize: theme.fontPixelSizeLarge
                elide: Text.ElideRight
                color: theme.fontColorNormal
                text: qsTr("Edit email settings manually")
            }
            Image {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10
                source: "image://theme/arrow-right"
            }
        }
    }

    ModalMessageBox {
        id: verifyCancel
        acceptButtonText: qsTr ("Yes")
        cancelButtonText: qsTr ("No")
        title: qsTr ("Discard changes")
        text: qsTr ("You have made changes to your settings. Are you sure you want to cancel?")
        onAccepted: { settingsPage.state = settingsPage.getHomescreen() }
    }
    ModalMessageBox {
        id: errorDialog
        acceptButtonText: qsTr("OK")
        showCancelButton: false
        title: qsTr("Error")
        text: qsTr("Error %1: %2").arg(emailAccount.errorCode).arg(emailAccount.errorMessage)
        onAccepted: {
            settingsPage.state = "ManualScreen";
            loader.item.message = qsTr("Sorry, we can't automatically set up your account. Please fill in account details:");
        }
    }

    // spinner overlay
    ModalSpinner { id: spinner }

    SaveRestoreState {
        id: detailsSaveRestoreState
        onSaveRequired: {
            setValue("email-details-verifyCancel-visible",verifyCancel.visible);
            setValue("email-details-errorDialog-visible",errorDialog.visible);
            sync();
        }
    }

    Component.onCompleted: {
        if(detailsSaveRestoreState.restoreRequired) {
            if(detailsSaveRestoreState.restoreOnceAndRemove("email-details-verifyCancel-visible","false") == "true") {
                verifyCancel.show();
            } else if(detailsSaveRestoreState.value("email-details-errorDialog-visible","false") == "true") {
                errorDialog.show();
            }
        }
    }
}

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
import MeeGo.App.Email 0.1

Item {
    Rectangle {
        anchors.fill: parent
        color: "#eaf6fb"
    }

    height: contentCol.height

    onHeightChanged: {
        settingsPage.height = height;
    }

    Column {
        id: contentCol
        width: settingsPage.width
        ControlGroup {
            id: content
            children: [
                Item { width: 1; height: 20; },
                TextControl { //this is a read-only element just so the user can see what preset was set
                    label: qsTr("Account description:")
                    Component.onCompleted: setText(emailAccount.description);
                    enabled: false
                    // hide this field for "other" type accounts
                    visible: emailAccount.preset != 0
                    id: descriptField
                },
                TextControl {
                    id: nameField
                    label: qsTr("Your name:")
                    Component.onCompleted: setText(emailAccount.name) //done to supress onTextChanged
                    onTextChanged: emailAccount.name = text
                    //KNOWN BUG: if you get an error, and then switch to a new langauge in the middle of setup (can you?), the error text does not update
                    errorText: registerSaveRestoreState.restoreRequired ?
                                   registerSaveRestoreState.restoreOnceAndRemove("email-register-nameField-errorText","") : ""
                },
                TextControl {
                    id: addressField
                    label: qsTr("Email address:")
                    Component.onCompleted: setText(emailAccount.address)
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhEmailCharactersOnly
                    onTextChanged: emailAccount.address = text
                    errorText: registerSaveRestoreState.restoreRequired ?
                                   registerSaveRestoreState.restoreOnceAndRemove("email-register-addressField-errorText","") : ""
                },
                PasswordControl {
                    id: passwordField
                    label: qsTr("Password:")
                    Component.onCompleted: setText(emailAccount.password)
                    onTextChanged: emailAccount.password = text
                    //errorText: registerSaveRestoreState.restoreRequired ? //TODO: make the password encrypted such that its not stored in plain-text
                    //               registerSaveRestoreState.value("email-register-passwordField-errorText") : ""
                },
                Item { width: 1; height: 40; }
            ]
        }
        BottomToolBar {
            width: settingsPage.width
            content: BottomToolBarRow {
                leftContent: Button {
                    text: qsTr("Next")
                    function validate() {
                        var errors = 0;
                        if (nameField.text.length === 0) {
                            nameField.errorText = qsTr("This field is required");
                            errors++;
                        } else {
                            nameField.errorText = "";
                        }
                        if (addressField.text.length === 0) {
                            addressField.errorText = qsTr("This field is required");
                            errors++;
                        } else {
                            addressField.errorText = "";
                        }
                        if (passwordField.text.length === 0) {
                            passwordField.errorText = qsTr("This field is required");
                            errors++;
                        } else {
                            passwordField.errorText = "";
                        }

/*
                        // Added By Daewon.Park
                        var accountList = accountListModel.getAllEmailAddresses();
                        for(var i = 0; i < accountList.length; i++) {
                            console.log("Account : " + addressField.text + " : " + accountList[i]);
                            if(addressField.text === accountList[i]) {
                                addressField.errorText = qsTr("Same account is already registered");
                                errors++;
                                break;
                            }
                        }
*/

                        return errors === 0;
                    }
                    onClicked: {
                        if (validate()) {

                            var emailAddress = addressField.text;
                            if (emailAddress.search(/yahoo.com/i) > 0)
                            {
                                emailAccount.preset = 3;
                                emailAccount.description = qsTr("Yahoo!");
                            }
                            else if ((emailAddress.search(/aim.com/i) > 0) || (emailAddress.search(/aol.com/i) > 0))
                            {
                                emailAccount.preset = 4; // AOL
                                emailAccount.description = qsTr("AOL");
                            }

                            emailAccount.applyPreset();
                            if (emailAccount.preset != 0) {
                                settingsPage.state = "DetailsScreen";
                            } else {
                                settingsPage.state = "ManualScreen";
                                loader.item.message = qsTr("Please fill in account details:");
                            }
                        }
                    }
                }
                rightContent:  Button {
                    text: qsTr("Cancel")
                    onClicked: {
                        verifyCancel.show();
                    }
                }
            }

            Component.onCompleted: {
                show();
            }
        }
    }

    ModalMessageBox {
        id: verifyCancel
        acceptButtonText: qsTr ("Yes")
        cancelButtonText: qsTr ("No")
        title: qsTr ("Discard changes")
        text: qsTr ("You have made changes to your settings. Are you sure you want to cancel?")
        onAccepted: {
            settingsPage.state = settingsPage.getHomescreen();
        }
    }
    // Added By Daewon.Park
    /*EmailAccountListModel {
        id : accountListModel
    }*/


    SaveRestoreState {
        id: registerSaveRestoreState
        onSaveRequired: {
            setValue("email-register-nameField-errorText",nameField.errorText);
            setValue("email-register-addressField-errorText",addressField.errorText);
            setValue("email-register-verifyCancel-visible",verifyCancel.visible);
            sync();
        }
    }

}

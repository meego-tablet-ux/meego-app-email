/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1
import MeeGo.App.Email 0.1

Item {
    id: folderListViewToolbar
    anchors.bottom: parent.bottom
    width: parent.width
    height: navigationBarImage.height

    property bool inEditMode: false

    signal editModeBegin();
    signal editModeEnd();

    BorderImage {
        id: navigationBarImage
        width: parent.width
        source: "image://meegotheme/widgets/common/action-bar/action-bar-background"
    }
    Item {
        anchors.fill: parent

        opacity: inEditMode == false ? 1 : 0

        ToolbarButton {
        id: composeButton
        anchors.left: parent.left
        anchors.top: parent.top
        iconName: "mail-compose"
            onClicked: {
                mailAttachmentModel.clear();
                folderListView.addApplicationPage(composer);
            }
        }
        Image {
            id: division1
            anchors.left: composeButton.right
            anchors.top: parent.top
            height: parent.height
            source: "image://theme/email/div"
        }

        ToolbarButton {
        id: editButton
        anchors.left: division1.right
        anchors.top: parent.top
        iconName: "mail-editlist"
            onClicked: {
                folderListViewToolbar.editModeBegin();
                inEditMode = true;
            }
        }
        Image {
            anchors.left: editButton.right
            anchors.top: parent.top
            height: parent.height
            source: "image://theme/email/div"
        }

        Image {
            id: division3
            anchors.right: refreshButton.left
            anchors.top: parent.top
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            source: "image://theme/email/div"
        }

        Item {
            id:refreshButton 
            anchors.right: parent.right
            anchors.top: parent.top
            height: refreshImage.height
            width: refreshImage.width
            Image {
                id: refreshImage
                anchors.centerIn: parent
                opacity: scene.refreshInProgress ? 0 : 1
                source: "image://meegotheme/icons/actionbar/view-sync"
            }

            EmailSpinner {
                id: spinner
                anchors.centerIn: parent
                opacity: scene.refreshInProgress ? 1 : 0
                spinning: scene.refreshInProgress
                maxSpinTime: 3600000
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (scene.refreshInProgress == true)
                    {
			mailAccountListModel.cancelOperations();
                        scene.refreshInProgress = false;
                    }
                    else
                    {
			mailAccountListModel.sendReceive();
                        scene.refreshInProgress = true;
                    }
                }
            }
        }
    }
    Item {
        anchors.fill: parent
        opacity: inEditMode == true ? 1 : 0

        ToolbarButton {
        id: deleteButton
        anchors.left: parent.left
        anchors.top: parent.top
        iconName: "edit-delete"
            onClicked: {
                messageListModel.deleteSelectedMessageIds();
                folderListContainer.numOfSelectedMessages = 0;
            }
        }
        Text {
            anchors.left: deleteButton.right
            anchors.verticalCenter: parent.verticalCenter
            id: numberOfSelectedMessages
            color: "white"
            //: Arg1 is the number of selected messages
            text: qsTr("(%1)").arg(folderListContainer.numOfSelectedMessages)
        }
        Image {
            id: separator1
            anchors.left: numberOfSelectedMessages.right
            anchors.leftMargin: 15
            anchors.top: parent.top
            height: parent.height
            source: "image://theme/email/div"
        }
        Image {
            id: separator2
            anchors.right: exitEditModeButton.left
            anchors.top: parent.top
            height: parent.height
            source: "image://theme/email/div"
        }
        Item {
            // FIX ME:  use the old icon until UX design team provide new ones.
            id: exitEditModeButton
            anchors.right: parent.right
            anchors.top: parent.top
            width: 65
            height:55
            property string iconName: "image://theme/email/icns_export/icn_can_edit_up"

            Image {
                id: settingsIcon
                anchors.centerIn: exitEditModeButton
                source: exitEditModeButton.iconName
                width: 48
                height: 48
                NumberAnimation on rotation {
                    id: imageRotation
                    running: false
                    from: 0; to: 360
                    loops: Animation.Infinite;
                    duration: 2400
                }
            }
            MouseArea {
                anchors.fill: parent
                onPressed : {
                    exitEditModeButton.iconName = "image://theme/email/icns_export/icn_can_edit_dn";
                }
                onReleased: {
                    exitEditModeButton.iconName = "image://theme/email/icns_export/icn_can_edit_up";
                }
                onClicked: {
                    folderListViewToolbar.editModeEnd();
                    inEditMode = false;
                }
            }
        }
    }
}

/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1

Item {
    id: addAttachmentPageContainer

    parent: addAttachmentPage.content
    width: scene.content.width
    height: parent.height

    property ListModel attachments

    function pickerCancelled () {
        console.log ("Cancelled picker");
    }

    function pickerSelected (uri) {
        console.log (uri + " selected");

        attachments.append ({"uri": uri});
    }

    function addPicker (pickerComponent) {
        var picker = pickerComponent.createObject (addAttachmentPageContainer);
        picker.show ();
        picker.cancel.connect (pickerCancelled);
        picker.selected.connect (pickerSelected);
    }

    Rectangle {
        width: parent.width
        anchors.top: parent.top
        anchors.bottom: toolbar.top
        color: "white"

        Column {
            anchors.fill: parent
/*
            Button {
                title: qsTr ("Documents")
                width: parent.width
                height: 40

                onClicked: {
                    addPicker (documentPicker);
                }
            }
*/
            Button {
                title: qsTr ("Photos")
                width: parent.width
                height: 40

                onClicked: {
                    addPicker (photoPicker);
                }
            }

            Button {
                title: qsTr ("Movies")
                width: parent.width
                height: 40

                onClicked: {
                    addPicker (moviePicker);
                }
            }

            Button {
                title: qsTr ("Music")
                width: parent.width
                height: 40

                onClicked: {
                    addPicker (musicPicker);
                }
            }
        }
    }

    AddAttachmentToolbar {
        id: toolbar
        width: parent.width
        anchors.bottom: parent.bottom

        onOkay: {
            scene.previousApplicationPage ();
        }
    }

    Component {
        id: documentPicker
        Rectangle {
            anchors.fill: parent
            color: "pink"
        }
    }

    Component {
        id: musicPicker
        MusicPicker {
            anchors.fill: parent

            showPlaylists: false
            showAlbums: false

            signal selected (string uri)
            onSongSelected: {
                console.log ("Song: " + song);
                selected (song);
            }
        }
    }

    Component {
        id: photoPicker
        PhotoPicker {
            anchors.fill: parent

            signal selected (string uri)

            onPhotoSelected: {
                console.log ("Photo: " + uri);
                selected (uri);
            }
        }
    }

    Component {
        id: moviePicker
        VideoPicker {
            anchors.fill: parent

            signal selected (string uri)

            onVideoSelected: {
                console.log ("Video: " + uri);
                selected (uri);
            }
        }
    }
}

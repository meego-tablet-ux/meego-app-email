/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1

//Model used for this Drop Down is a StringList Model
//Reset the "dataModel" with appropriate values to make this work
Item {
    id:dropDown

    width: 350
    height: 53
    property int selectedIndex:0
    property variant selectedValue // This is writeonly.It does not get updated
    property Component delegateComponent: Text {
        property string data
        text: data
    }

    signal selectionChanged (int index, variant data)

    //Set this property with the appropriate values
    property variant dataModel: null
    property variant dataList: null

    property Item selectedDelegate: null

    function setFromModel () {
        if (delegateComponent) {
            selectedDelegate = delegateComponent.createObject (comboBoxContents);
            selectedDelegate.data = dataModel.get (selectedIndex);

            selectedDelegate.anchors.verticalCenter = comboBoxContents.verticalCenter;
        }
    }

    function setFromList () {
        if (delegateComponent) {
            selectedDelegate = delegateComponent.createObject (comboBoxContents);
            selectedDelegate.data = dataList[selectedIndex];
            selectedDelegate.anchors.verticalCenter = comboBoxContents.verticalCenter;
        }
    }

    onSelectedIndexChanged: {
        if (dataModel == null && dataList == null) {
            return;
        }

        if (selectedDelegate) {
            selectedDelegate.destroy ();
            selectedDelegate = null;
        }

        if (dataModel) {
            setFromModel ();
        } else {
            setFromList ();
        }
    }

    function setSelectedValue (value) {
        if (dataModel == null && dataList == null) {
            return;
        }

        if (dataModel) {
            for (var i = 0; i < dataModel.count; i++) {
                if (dataModel.get (i) == value) {
                    selectedIndex = i;
                    return;
                }
            }

            console.log ("WARNING: Setting selectedValue does not work for elements with dataModel set");
            // Couldn't find it in the dataModel
            return;
        }

        if (dataList) {
            for (var i = 0; i < dataList.length; i++) {
                if (dataList[i] == value) {
                    selectedIndex = i;
                    return;
                }
            }

            return;
        }
    }

    onSelectedValueChanged: {
        setSelectedValue (selectedValue);
    }

    onDelegateComponentChanged: {
        console.log ("Component set");
        if (dataModel == null && dataList == null) {
            return;
        }

        if (selectedDelegate) {
            selectedDelegate.destroy ();
            selectedDelegate = null;
        }

        if (dataModel) {
            setFromModel ();
        } else {
            setFromList ();
        }
    }

    onDataModelChanged: {
        if (dataModel == null) {
            return;
        }

        if (selectedDelegate) {
            selectedDelegate.destroy ();
            selectedDelegate = null;
        }
        setFromModel ();
    }

    onDataListChanged: {
        if (dataList == null) {
            return;
        }

        if (selectedDelegate) {
            selectedDelegate.destroy ();
            selectedDelegate = null;
        }
        setFromList ();
    }

    Item {
        id: outerBox
        width: parent.width
        height: parent.height

        Image {
            id: leftIcon
            anchors.left: parent.left
            anchors.top: parent.top
            width:20
            smooth: true
            source:"image://theme/dropdown_white_50px_1"
        }

        Image {
            id: rightIcon
            anchors.top: parent.top
            anchors.right: parent.right
            smooth: true
            source:"image://theme/dropdown_white_50px_3"
        }

        Image {
            id: centerIcon
            source: "image://theme/dropdown_white_60px_2"
            anchors.top: parent.top
            anchors.left: leftIcon.right
            anchors.right: rightIcon.left
            smooth: true
        }

        // This is the parent for our selected delegate
        Item {
            id: comboBoxContents
            anchors.fill: parent
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                var map = mapToItem (scene.content, mouse.x, parent.height);
                dropdownMenu.mouseX = map.x;
                dropdownMenu.mouseY = map.y;
                dropdownMenu.visible = true;
            }
        }

        AbstractContext {
            id: dropdownMenu

            content: DropDownList {
                id: listview
                minWidth: 200
                maxWidth: dropDown.width
                model: dataModel ? dataModel:dataList
                currentIndex: dropDown.selectedIndex

                delegateComponent: dropDown.delegateComponent

                onTriggered: {
                    dropdownMenu.visible = false;
                    dropDown.selectedIndex = index;
                    dropDown.selectionChanged (index, data);
                }
            }
        }
    }
}


/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * LGPL, version 2.1.  The full text of the LGPL Licence is at
 * http://www.gnu.org/licenses/lgpl.html
 */

import Qt 4.7

import MeeGo.Ux.Kernel 0.1
import MeeGo.Components 0.1
import MeeGo.App.Email 0.1

// the focus scope ensures that only one item actually gets the focus
FocusScope {
    id: scope

    property alias html:                edit.html

    property alias modified:            edit.modified
    property alias contentsScale:       edit.contentsScale
    property alias delegateLinks:       edit.delegateLinks
    property alias font:                edit.font
    property alias contentsTimeoutMs:   edit.contentsTimeoutMs
    property alias readOnly:            edit.readOnly

    signal linkClicked( string url )
    signal loadStarted()
    signal loadFinished( bool ok )
    signal loadProgress( int progress )

    function startZooming() { edit.startZooming(); }
    function stopZooming()  { edit.stopZooming(); }

    function setFocusElement( elementName ) { edit.setFocusElement(elementName); container.focus = true; }

    property alias contentHeight:       flick.contentHeight

    property alias source:              container.source
    property alias border:              container.border
    property alias progress:            container.progress
    property alias smooth:              container.smooth
    property alias status:              container.status
    property alias horizontalTileMode:  container.horizontalTileMode
    property alias verticalTileMode:    container.verticalTileMode
    property alias defaultSource:       container.defaultSource
    property alias isValidSource:       container.isValidSource

    signal htmlChanged
    signal cursorRectangleChanged

    height: 50
    width: 200

    function forceFocus() {
        edit.forceFocus()
    }

    function paste() {
        edit.paste()
    }

    function showContextMenu() {
        edit.contextMenu()
    }

    onHeightChanged: {
        if( flick.contentHeight <= flick.height ){
            flick.contentY = 0
        }
        else if( flick.contentY + flick.height > flick.contentHeight ){
            flick.contentY = flick.contentHeight - flick.height
        }
    }

    GestureArea {     // this ensures the html gets focus via FocusScope when only the ThemeImage is clicked
        anchors.fill: parent
        acceptUnhandledEvents: true

        Tap {
            onStarted: {
                scope.focus = true
            }
        }
    }

    ThemeImage {
        id: container

        anchors.fill: parent
        clip: true
//        source: "image://themedimage/widgets/common/text-area/text-area-background"

        //        source: (edit.activeFocus && !edit.readOnly) ? "image://themedimage/widgets/common/text-area/text-area-background-active" : "image://themedimage/widgets/common/text-area/text-area-background"
        //        opacity: edit.readOnly ? 0.5 : 1.0

        Theme { id: theme }

        Flickable {
            id: flick
            clip: true

            function ensureVisible(r) {
                if (contentX >= r.x){
                    contentX = r.x
                } else if (contentX+width <= r.x+r.width) {
                    contentX = r.x+r.width-width
                }

                if (contentY >= r.y){
                    contentY = r.y
                } else if (contentY+height <= r.y+r.height) {
                    contentY = r.y+r.height-height
                }
            }

            anchors {
                fill: parent
                topMargin: 3      // this value is the hardcoded margin to keep the text within the backgrounds text field
                bottomMargin: 3
                leftMargin: 4
                rightMargin: 4
            }

            contentWidth: edit.width
            contentHeight: edit.height

            property variant centerPoint


            interactive: ( contentHeight > flick.height || contentWidth > flick.width) ? true : false

            GestureArea {
                id: webGestureArea
                anchors.fill: parent
                acceptUnhandledEvents:  true

                // Pinch handles zooming.
                Pinch {
                    id: webpinch
                    property real startScale: 1.0;
                    property real minScale: 0.5;
                    property real maxScale: 5.0;

                    onStarted: {
                        //                    console.log("\nPinch Started")
                        flick.interactive = false;
                        flick.centerPoint = window.mapToItem(flick, gesture.centerPoint.x, gesture.centerPoint.y);
                        startScale = edit.contentsScale;
                        edit.startZooming();
                        //                    console.log("Pinch Start End...\n")
                    }

                    onUpdated: {
                        var cw = flick.contentWidth;
                        var ch = flick.contentHeight;

                        edit.contentsScale = Math.max(minScale, Math.min(startScale * gesture.totalScaleFactor, maxScale));

                        flick.contentX = (flick.centerPoint.x + flick.contentX) / cw * flick.contentWidth - flick.centerPoint.x;
                        flick.contentY = (flick.centerPoint.y + flick.contentY) / ch * flick.contentHeight - flick.centerPoint.y;
                    }

                    onFinished: {
                        edit.stopZooming();
                        flick.interactive = true;

                    }
                }

                TapAndHold {
                    onFinished: {
                        var p = gesture.position
                        contextMenu.setPosition( mapToItem( topItem.topItem, p.x, p.y ).x, mapToItem( topItem.topItem, p.x, p.y ).y )
                        contextMenu.updateAndShow()

                        console.log("--- =========== End Tap And Hold =============\n\n")
                    }
                }
            }

            HtmlField {
                id: edit
                focus: true

                Component.onDestruction: {
                    if( activeFocus ) {
                        topItem.topItem.forceActiveFocus()
                    }
                }

                preferredWidth: flick.width
                preferredHeight: flick.height

                font.pixelSize: theme.fontPixelSizeNormal
                Keys.forwardTo: scope

                onHtmlChanged: {
                    scope.htmlChanged()
                }

                onGeometryChanged: {
                    if( window ) {
                        if( edit.activeFocus && window.currentVkbHeight > 0 ) {
                            window.updateVkbShift( mapToItem( topItem.topItem, 0, edit.cursorRectangle.y + edit.cursorRectangle.height * 3 ).y )
                        }
                    }
                }

                onLinkClicked: { scope.linkClicked( url ) }
                onLoadStarted: { scope.loadStarted() }
                onLoadFinished: { scope.loadFinished( ok ) }
                onLoadProgress: { scope.loadProgress( progress ) }

                onFocusChanged: {
                    console.log( "||| FOCUS HtmlField.onFocusChanged = " + focus + " activeFocus = " + activeFocus )
                }

                onActiveFocusChanged: {
                    console.log( "||| FOCUS HtmlField.onACTIVEFocusChanged = " + focus + " activeFocus = " + activeFocus )
                }
        }

        TopItem{ id: topItem }


        ContextMenu {
            id: contextMenu

            forceFingerMode: -1
            shouldFocus: false

            // NOTE: These are not translated on purpose.
            property variant actionIds: [ "Copy" , "Cut", "Past" ]

            function updateAndShow()
            {

                var tmpIds = new Array()
                var tmpModel = new Array()

                actionIds = []
                if ( edit.copyAvailable() ) {
                    tmpModel.push(qsTr("Copy"))
                    tmpIds.push("Copy")
                }

                if ( edit.cutAvailable() ) {
                    tmpModel.push(qsTr("Cut"))
                    tmpIds.push("Cut")
                }

                if ( edit.pasteAvailable() ) {
                    tmpModel.push(qsTr("Paste"))
                    tmpIds.push("Paste")
                }

                clipboardMenu.model = tmpModel
                actionIds = tmpIds

                show()
            }

            content: ActionMenu{
                id: clipboardMenu

                maxWidth: 200
                minWidth: 100

                model: [ qsTr("Copy"), qsTr("Cut"), qsTr("Paste") ]

                onTriggered: {
                    // Using actionIds as key instead of model so I
                    // don't have to worry about translation.
                    var triggeredId = contextMenu.actionIds[index]

                    if (triggeredId == "Copy") {
                        edit.copy()
                    } else if (triggeredId == "Cut") {
                        edit.cut()
                    } else if (triggeredId == "Paste") {
                        edit.paste()
                    }

                    contextMenu.hide()
                }
            }
        }

        Connections {
            target: mainWindow
            onVkbHeight: {
                if( window ) {
                    if( edit.activeFocus && scope.height > 0 ) {
                        window.adjustForVkb( mapToItem( topItem.topItem, 0, edit.cursorRectangle.y + edit.cursorRectangle.height * 3 ).y, width, height )
                    }
                }
            }
        }
    }

    }
}

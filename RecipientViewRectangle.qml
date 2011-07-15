import QtQuick 1.0

Rectangle {
    id: container

    property variant recipients
    property string recipientLabel

    height: 50
    width: parent.width

    Image {
        anchors.fill: parent
        fillMode: Image.Tile
        source: "image://theme/email/bg_email details_l"
    }


    Row {
        spacing: 5
        height: parent.height
        width: parent.width
        anchors.left: parent.left
        anchors.leftMargin: 3

        Text {
            id: recipientText
            width: subjectLabel.width
            font.pixelSize: theme.fontPixelSizeMedium
            text: container.recipientLabel
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
        }

        Flickable {
            id: flicker

            height: parent.height
            width: parent.width - recipientText.width - 5

            contentWidth:  recipientRow.width
            contentHeight: flicker.height

            clip: true
            interactive: contentWidth > width
            flickableDirection: Flickable.HorizontalFlick

            Row {
                id: recipientRow

                anchors.verticalCenter: parent.verticalCenter

                spacing: 5

                Repeater {
                    model: container.recipients

                    EmailAddress {
                        // Don't show an empty "pill"
                        //
                        // @todo Why do we sometimes get an empty string from the Repeater?
                        visible: index < recipients.length && container.recipients[index] != ""
                        emailAddress: container.recipients[index]
                    }
                }
            }
        }
    }
}

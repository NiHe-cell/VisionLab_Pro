import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string title: ""
    default property alias content: body.data

    color: Theme.card
    radius: Theme.radiusMd
    border.color: Theme.border
    border.width: 1
    implicitHeight: layout.implicitHeight + Theme.spaceXl * 2

    ColumnLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        Text {
            text: root.title
            color: Theme.foreground
            font.pixelSize: Theme.h2
            font.family: Theme.fontUi
            Layout.fillWidth: true
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: Theme.spaceMd
        }
    }
}

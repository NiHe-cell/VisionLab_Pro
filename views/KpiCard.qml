import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string label: ""
    property string value: ""

    color: Theme.card
    radius: Theme.radiusMd
    border.color: Theme.border
    border.width: 1
    implicitHeight: 88

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceSm

        Text {
            text: root.label
            color: Theme.mutedForeground
            font.pixelSize: Theme.caption
            font.family: Theme.fontUi
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Text {
            text: root.value
            color: Theme.foreground
            font.pixelSize: Theme.h2
            font.family: Theme.fontMono
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }
}

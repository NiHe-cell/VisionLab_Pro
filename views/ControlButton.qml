import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property string glyph: ""
    property bool destructive: false

    implicitWidth: Theme.hit
    implicitHeight: Theme.hit
    font.pixelSize: Theme.caption
    font.family: Theme.fontUi
    activeFocusOnTab: true
    Accessible.role: Accessible.Button

    contentItem: Text {
        text: root.glyph
        color: (root.destructive && root.hovered) ? Theme.textOnDestructive : Theme.foreground
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 6
        color: {
            if (root.destructive && (root.hovered || root.pressed))
                return Theme.destructive
            if (root.hovered || root.pressed)
                return Theme.muted
            return "transparent"
        }
        border.width: root.activeFocus ? 2 : 0
        border.color: Theme.ring
    }
}

import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property bool selected: false
    property bool primary: false
    property bool danger: false

    implicitHeight: Theme.hit
    leftPadding: Theme.spaceLg
    rightPadding: Theme.spaceLg
    font.pixelSize: Theme.caption
    font.family: Theme.fontUi
    activeFocusOnTab: true

    contentItem: Text {
        text: root.text
        color: {
            if (root.primary || root.selected)
                return Theme.textOnAccent
            if (root.danger)
                return Theme.textOnDestructive
            return Theme.foreground
        }
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radiusSm
        color: {
            if (!root.enabled)
                return Theme.muted
            if (root.danger)
                return root.hovered ? Qt.darker(Theme.destructive, 1.1) : Theme.destructive
            if (root.primary || root.selected)
                return Theme.accent
            if (root.hovered || root.pressed)
                return Theme.muted
            return Theme.card
        }
        border.width: (root.primary || root.selected || root.danger) ? 0 : 1
        border.color: root.activeFocus ? Theme.ring : Theme.border
        opacity: root.enabled ? 1 : 0.45
    }
}

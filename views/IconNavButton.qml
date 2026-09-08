import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    required property int pageIndex
    required property url iconSource
    required property string accessibleName

    readonly property bool selected: VisionController.currentPage === pageIndex

    display: AbstractButton.IconOnly
    icon.source: root.iconSource
    icon.width: Theme.icon
    icon.height: Theme.icon
    icon.color: root.selected ? Theme.accent : Theme.mutedForeground
    implicitWidth: 40
    implicitHeight: 40
    Accessible.name: root.accessibleName
    Accessible.role: Accessible.Button
    Accessible.checkable: true
    Accessible.checked: root.selected
    ToolTip.visible: hovered
    ToolTip.delay: 400
    ToolTip.text: root.accessibleName
    activeFocusOnTab: true

    background: Rectangle {
        radius: Theme.radiusSm
        color: (root.selected || root.hovered) ? Theme.muted : "transparent"
        border.width: root.activeFocus ? 2 : 0
        border.color: Theme.ring

        Rectangle {
            visible: root.selected
            width: 3
            height: parent.height - 8
            radius: 1
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 3
            color: Theme.accent
        }
    }

    onClicked: VisionController.currentPage = pageIndex
}

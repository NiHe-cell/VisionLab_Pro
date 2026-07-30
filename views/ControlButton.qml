import QtQuick
import QtQuick.Controls

Button {
    id: root
    width: 15
    height: 15
    flat: true

    signal clicked()

    property color hoverColor: "#3a3a3a"
    property color backgroundColor: "#fff"

    background: Rectangle {
        id: rect
        radius: 100
        color: root.backgroundColor
        opacity: buttonMouseArea.containsMouse ? 0.75 : 1;
    }

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            root.clicked()
        }
    }
}

import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0F172B"
    width: 168

    readonly property var labels: [
        I18n.navMonitor,
        I18n.navEvents,
        I18n.navPerformance,
        I18n.navSettings
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 6

        Repeater {
            model: 4

            Rectangle {
                required property int index
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: 8
                color: VisionController.currentPage === index ? "#74D4FF" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: root.labels[index]
                    color: VisionController.currentPage === index ? "#0F172B" : "#E2E8F0"
                    font.pixelSize: 13
                    font.bold: VisionController.currentPage === index
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: VisionController.currentPage = index
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

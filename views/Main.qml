import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: root
    width: 1200
    height: 900
    visible: true
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"

    Rectangle {
        id: appFrame
        anchors.fill: parent
        color: "#f8fafc"
        radius: 12
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TitleBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                z: 10
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                SideNav {
                    Layout.preferredWidth: 168
                    Layout.fillHeight: true
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: VisionController.currentPage

                    MonitorPage {}
                    EventsPage {}
                    PerformancePage {}
                    SettingsPage {}
                }
            }
        }
    }
}

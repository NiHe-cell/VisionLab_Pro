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
    palette.window: Theme.background
    palette.windowText: Theme.foreground
    palette.base: Theme.card
    palette.alternateBase: Theme.muted
    palette.text: Theme.foreground
    palette.button: Theme.muted
    palette.buttonText: Theme.foreground
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.textOnAccent
    palette.placeholderText: Theme.mutedForeground
    palette.mid: Theme.border
    palette.dark: Theme.primary
    palette.light: Theme.muted
    palette.midlight: Theme.card
    palette.shadow: "#000000"

    Rectangle {
        id: appFrame
        anchors.fill: parent
        color: Theme.background
        radius: Theme.radiusMd
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TitleBar {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.titleBar
                z: 10
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                SideNav {
                    Layout.preferredWidth: Theme.rail
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

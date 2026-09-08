import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    color: Theme.primary
    implicitWidth: Theme.rail

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spaceXl
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        spacing: Theme.spaceSm

        IconNavButton {
            Layout.alignment: Qt.AlignHCenter
            pageIndex: 0
            iconSource: Qt.resolvedUrl("../assets/icons/video.svg")
            accessibleName: I18n.navMonitor
        }
        IconNavButton {
            Layout.alignment: Qt.AlignHCenter
            pageIndex: 1
            iconSource: Qt.resolvedUrl("../assets/icons/warning.svg")
            accessibleName: I18n.navEvents
        }
        IconNavButton {
            Layout.alignment: Qt.AlignHCenter
            pageIndex: 2
            iconSource: Qt.resolvedUrl("../assets/icons/pulse.svg")
            accessibleName: I18n.navPerformance
        }
        IconNavButton {
            Layout.alignment: Qt.AlignHCenter
            pageIndex: 3
            iconSource: Qt.resolvedUrl("../assets/icons/gear.svg")
            accessibleName: I18n.navSettings
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

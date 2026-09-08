import QtQuick
import QtQuick.Layouts

Item {
    id: root

    function typeColor(type) {
        if (type === 1)
            return "#EAB308"
        if (type === 2)
            return "#F97316"
        if (type === 3)
            return "#38BDF8"
        return Theme.accent
    }

    readonly property var filters: [
        { label: I18n.eventAll, value: -1 },
        { label: I18n.eventRoi, value: 0 },
        { label: I18n.eventLine, value: 1 },
        { label: I18n.eventLoiter, value: 2 },
        { label: I18n.eventCount, value: 3 }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceLg

            Text {
                text: I18n.navEvents
                font.pixelSize: Theme.h1
                font.family: Theme.fontUi
                font.bold: true
                color: Theme.foreground
            }

            Item {
                Layout.fillWidth: true
            }

            Repeater {
                model: root.filters

                ToolChip {
                    required property var modelData
                    text: modelData.label
                    selected: VisionController.eventTypeFilter === modelData.value
                    onClicked: VisionController.eventTypeFilter = modelData.value
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.row
            color: Theme.card
            radius: Theme.radiusSm
            border.color: Theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceMd

                Text { Layout.preferredWidth: 160; text: I18n.colTime; font.bold: true; color: Theme.mutedForeground; font.pixelSize: Theme.caption; font.family: Theme.fontUi }
                Text { Layout.preferredWidth: 88; text: I18n.colType; font.bold: true; color: Theme.mutedForeground; font.pixelSize: Theme.caption; font.family: Theme.fontUi }
                Text { Layout.preferredWidth: 88; text: I18n.colRule; font.bold: true; color: Theme.mutedForeground; font.pixelSize: Theme.caption; font.family: Theme.fontUi }
                Text { Layout.preferredWidth: 72; text: I18n.colTrack; font.bold: true; color: Theme.mutedForeground; font.pixelSize: Theme.caption; font.family: Theme.fontUi }
                Text { Layout.preferredWidth: 88; text: I18n.colLabel; font.bold: true; color: Theme.mutedForeground; font.pixelSize: Theme.caption; font.family: Theme.fontUi }
                Text { Layout.fillWidth: true; text: I18n.colMessage; font.bold: true; color: Theme.mutedForeground; font.pixelSize: Theme.caption; font.family: Theme.fontUi }
            }
        }

        ListView {
            id: eventList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spaceXs
            boundsBehavior: Flickable.StopAtBounds
            model: VisionController.eventFilterModel

            delegate: Rectangle {
                required property string timeText
                required property int type
                required property string ruleId
                required property var trackId
                required property string label
                required property string message
                required property int index

                width: eventList.width
                height: Theme.row
                radius: 6
                color: index % 2 === 0 ? Theme.card : Theme.muted

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceLg
                    anchors.rightMargin: Theme.spaceLg
                    spacing: Theme.spaceMd

                    Text {
                        Layout.preferredWidth: 160
                        text: timeText
                        color: Theme.foreground
                        font.pixelSize: Theme.caption
                        font.family: Theme.fontMono
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        Layout.preferredWidth: 88
                        Layout.preferredHeight: 22
                        radius: 11
                        color: Qt.rgba(0, 0, 0, 0)
                        border.width: 1
                        border.color: root.typeColor(type)

                        Text {
                            anchors.centerIn: parent
                            text: I18n.eventTypeLabel(type)
                            color: root.typeColor(type)
                            font.pixelSize: Theme.caption
                            font.family: Theme.fontUi
                        }
                    }

                    Text { Layout.preferredWidth: 88; text: ruleId; color: Theme.foreground; font.pixelSize: Theme.caption; font.family: Theme.fontUi; elide: Text.ElideRight }
                    Text { Layout.preferredWidth: 72; text: String(trackId); color: Theme.foreground; font.pixelSize: Theme.caption; font.family: Theme.fontMono }
                    Text { Layout.preferredWidth: 88; text: label; color: Theme.foreground; font.pixelSize: Theme.caption; font.family: Theme.fontUi; elide: Text.ElideRight }
                    Text { Layout.fillWidth: true; text: message; color: Theme.foreground; font.pixelSize: Theme.caption; font.family: Theme.fontUi; elide: Text.ElideRight }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: eventList.count === 0
                text: I18n.eventsEmpty
                color: Theme.mutedForeground
                font.pixelSize: Theme.body
                font.family: Theme.fontUi
            }
        }
    }
}

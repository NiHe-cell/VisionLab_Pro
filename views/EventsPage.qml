import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: I18n.navEvents
                font.pixelSize: 18
                font.bold: true
                color: "#0F172B"
            }

            Item {
                Layout.fillWidth: true
            }

            ComboBox {
                id: typeFilter
                Layout.preferredWidth: 160
                model: [
                    { text: I18n.eventAll, value: -1 },
                    { text: I18n.eventRoi, value: 0 },
                    { text: I18n.eventLine, value: 1 },
                    { text: I18n.eventLoiter, value: 2 },
                    { text: I18n.eventCount, value: 3 }
                ]
                textRole: "text"
                currentIndex: 0
                onActivated: VisionController.eventTypeFilter = model[currentIndex].value
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { Layout.preferredWidth: 160; text: I18n.colTime; font.bold: true; color: "#334155"; font.pixelSize: 12 }
            Text { Layout.preferredWidth: 72; text: I18n.colType; font.bold: true; color: "#334155"; font.pixelSize: 12 }
            Text { Layout.preferredWidth: 88; text: I18n.colRule; font.bold: true; color: "#334155"; font.pixelSize: 12 }
            Text { Layout.preferredWidth: 72; text: I18n.colTrack; font.bold: true; color: "#334155"; font.pixelSize: 12 }
            Text { Layout.preferredWidth: 88; text: I18n.colLabel; font.bold: true; color: "#334155"; font.pixelSize: 12 }
            Text { Layout.fillWidth: true; text: I18n.colMessage; font.bold: true; color: "#334155"; font.pixelSize: 12 }
        }

        ListView {
            id: eventList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: VisionController.eventFilterModel

            delegate: RowLayout {
                required property string timeText
                required property int type
                required property string ruleId
                required property var trackId
                required property string label
                required property string message
                width: eventList.width
                spacing: 8

                Text { Layout.preferredWidth: 160; text: timeText; color: "#0F172B"; font.pixelSize: 12; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 72; text: I18n.eventTypeLabel(type); color: "#0F172B"; font.pixelSize: 12 }
                Text { Layout.preferredWidth: 88; text: ruleId; color: "#0F172B"; font.pixelSize: 12; elide: Text.ElideRight }
                Text { Layout.preferredWidth: 72; text: String(trackId); color: "#0F172B"; font.pixelSize: 12 }
                Text { Layout.preferredWidth: 88; text: label; color: "#0F172B"; font.pixelSize: 12; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; text: message; color: "#0F172B"; font.pixelSize: 12; elide: Text.ElideRight }
            }

            Text {
                anchors.centerIn: parent
                visible: eventList.count === 0
                text: I18n.eventsEmpty
                color: "#64748B"
                font.pixelSize: 14
            }
        }
    }
}

import QtQuick
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 24
        spacing: 8

        Text {
            text: I18n.navPerformance
            font.pixelSize: 18
            font.bold: true
            color: "#0F172B"
        }

        Text {
            text: "captureFps  " + VisionController.performanceModel.captureFps.toFixed(1)
            color: "#334155"
            font.pixelSize: 14
        }

        Text {
            text: "inferenceFps  " + VisionController.performanceModel.inferenceFps.toFixed(1)
            color: "#334155"
            font.pixelSize: 14
        }

        Text {
            text: "activeTracks  " + VisionController.performanceModel.activeTracks
            color: "#334155"
            font.pixelSize: 14
        }

        Text {
            text: "eventsEmitted  " + VisionController.performanceModel.eventsEmitted
            color: "#334155"
            font.pixelSize: 14
        }
    }
}

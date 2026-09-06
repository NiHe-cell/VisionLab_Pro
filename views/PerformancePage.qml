import QtQuick
import QtQuick.Layouts

Item {
    readonly property var stats: VisionController.performanceModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text {
            text: I18n.navPerformance
            font.pixelSize: 18
            font.bold: true
            color: "#0F172B"
        }

        GridLayout {
            columns: 2
            columnSpacing: 24
            rowSpacing: 8

            Text { text: I18n.statCaptureFps; color: "#64748B"; font.pixelSize: 13 }
            Text { text: stats.captureFps.toFixed(1); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statInferenceFps; color: "#64748B"; font.pixelSize: 13 }
            Text { text: stats.inferenceFps.toFixed(1); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statP50; color: "#64748B"; font.pixelSize: 13 }
            Text { text: stats.p50InferenceLatencyMs.toFixed(1); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statP95; color: "#64748B"; font.pixelSize: 13 }
            Text { text: stats.p95InferenceLatencyMs.toFixed(1); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statE2e; color: "#64748B"; font.pixelSize: 13 }
            Text { text: stats.endToEndLatencyMs.toFixed(1); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statQueue; color: "#64748B"; font.pixelSize: 13 }
            Text { text: String(stats.captureQueueDepth); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statDropped; color: "#64748B"; font.pixelSize: 13 }
            Text { text: String(stats.droppedFrames); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statTracks; color: "#64748B"; font.pixelSize: 13 }
            Text { text: String(stats.activeTracks); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statEvents; color: "#64748B"; font.pixelSize: 13 }
            Text { text: String(stats.eventsEmitted); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statRuleLatency; color: "#64748B"; font.pixelSize: 13 }
            Text { text: stats.avgRuleLatencyMs.toFixed(1); color: "#0F172B"; font.pixelSize: 13 }
            Text { text: I18n.statEnabledRules; color: "#64748B"; font.pixelSize: 13 }
            Text { text: String(stats.enabledRules); color: "#0F172B"; font.pixelSize: 13 }
        }

        Text {
            text: I18n.perfSnapshotHint
            color: "#94A3B8"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

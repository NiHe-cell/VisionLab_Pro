import QtQuick
import QtQuick.Layouts

Item {
    id: root

    readonly property var stats: VisionController.performanceModel
    property string snapshotTime: "--:--:--"

    Timer {
        interval: 250
        running: true
        repeat: true
        onTriggered: {
            if (VisionController.running)
                root.snapshotTime = Qt.formatTime(new Date(), "HH:mm:ss")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.space2xl
        spacing: Theme.spaceLg

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceLg

            Text {
                text: I18n.navPerformance
                font.pixelSize: Theme.h1
                font.family: Theme.fontUi
                font.bold: true
                color: Theme.foreground
            }

            Rectangle {
                Layout.preferredHeight: Theme.hit
                Layout.preferredWidth: liveText.implicitWidth + Theme.spaceXl
                radius: Theme.radiusSm
                color: Theme.muted
                border.width: 1
                border.color: VisionController.running ? Theme.accent : Theme.border

                Text {
                    id: liveText
                    anchors.centerIn: parent
                    text: (VisionController.running ? I18n.liveStatus : I18n.pausedStatus) + "  " + root.snapshotTime
                    color: VisionController.running ? Theme.accent : Theme.mutedForeground
                    font.pixelSize: Theme.caption
                    font.family: Theme.fontMono
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: Theme.spaceLg
            rowSpacing: Theme.spaceLg

            KpiCard { Layout.fillWidth: true; label: I18n.statCaptureFps; value: stats.captureFps.toFixed(1) }
            KpiCard { Layout.fillWidth: true; label: I18n.statInferenceFps; value: stats.inferenceFps.toFixed(1) }
            KpiCard { Layout.fillWidth: true; label: I18n.statP50; value: stats.p50InferenceLatencyMs.toFixed(1) }
            KpiCard { Layout.fillWidth: true; label: I18n.statP95; value: stats.p95InferenceLatencyMs.toFixed(1) }
            KpiCard { Layout.fillWidth: true; label: I18n.statE2e; value: stats.endToEndLatencyMs.toFixed(1) }
            KpiCard { Layout.fillWidth: true; label: I18n.statQueue; value: String(stats.captureQueueDepth) }
            KpiCard { Layout.fillWidth: true; label: I18n.statDropped; value: String(stats.droppedFrames) }
            KpiCard { Layout.fillWidth: true; label: I18n.statTracks; value: String(stats.activeTracks) }
            KpiCard { Layout.fillWidth: true; label: I18n.statEvents; value: String(stats.eventsEmitted) }
            KpiCard { Layout.fillWidth: true; label: I18n.statRuleLatency; value: stats.avgRuleLatencyMs.toFixed(1) }
            KpiCard { Layout.fillWidth: true; label: I18n.statEnabledRules; value: String(stats.enabledRules) }
        }

        Text {
            text: I18n.perfSnapshotHint
            color: Theme.mutedForeground
            font.pixelSize: Theme.caption
            font.family: Theme.fontUi
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

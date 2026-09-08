import QtQuick
import QtQuick.Layouts

Item {
    id: root

    readonly property string selectedMode: VisionController.mode.length > 0
                                           ? VisionController.mode
                                           : "Face Detection"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.commandBar
            color: Theme.card
            border.color: Theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceSm

                Repeater {
                    model: ["Face Detection", "Object Detection", "Motion Detection"]

                    ToolChip {
                        required property string modelData
                        text: I18n.modeLabel(modelData)
                        selected: root.selectedMode === modelData
                        onClicked: VisionController.setMode(modelData)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Repeater {
                    model: [
                        { label: I18n.toolRoi, tool: 1 },
                        { label: I18n.toolLine, tool: 2 },
                        { label: I18n.toolLoiter, tool: 3 },
                        { label: I18n.toolCount, tool: 4 }
                    ]

                    ToolChip {
                        required property var modelData
                        text: modelData.label
                        selected: VisionController.drawTool === modelData.tool
                        enabled: !VisionController.running
                        onClicked: VisionController.beginDraw(modelData.tool)
                    }
                }

                ToolChip {
                    text: I18n.applyRules
                    primary: true
                    enabled: !VisionController.running
                    onClicked: VisionController.commitRulesToEngine()
                }
            }
        }

        CameraView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spaceMd
        }
    }
}

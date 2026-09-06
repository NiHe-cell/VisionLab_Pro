import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string currentSelectedProcessor: "Face Detection"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            color: "#CAD5E2"
            border.color: "#d1d5db"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Item {
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: ["Face Detection", "Object Detection", "Motion Detection"]

                        Rectangle {
                            required property string modelData
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 28
                            radius: 8
                            color: root.currentSelectedProcessor === modelData ? "#74D4FF" : "#e5e7eb"
                            border.color: "#cad5e2"

                            Text {
                                anchors.centerIn: parent
                                text: I18n.modeLabel(modelData)
                                font.pixelSize: 12
                                color: root.currentSelectedProcessor === modelData ? "#fff" : "#1C69A8"
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.currentSelectedProcessor = modelData
                                    VisionController.setMode(modelData)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            { label: I18n.toolRoi, tool: 1 },
                            { label: I18n.toolLine, tool: 2 },
                            { label: I18n.toolLoiter, tool: 3 },
                            { label: I18n.toolCount, tool: 4 }
                        ]

                        Rectangle {
                            required property var modelData
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 28
                            radius: 8
                            opacity: VisionController.running ? 0.45 : 1
                            color: VisionController.drawTool === modelData.tool ? "#74D4FF" : "#e5e7eb"
                            border.color: "#cad5e2"

                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                font.pixelSize: 12
                                color: VisionController.drawTool === modelData.tool ? "#fff" : "#1C69A8"
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !VisionController.running
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: VisionController.beginDraw(modelData.tool)
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 88
                        Layout.preferredHeight: 28
                        radius: 8
                        opacity: VisionController.running ? 0.45 : 1
                        color: "#1C69A8"

                        Text {
                            anchors.centerIn: parent
                            text: I18n.applyRules
                            font.pixelSize: 12
                            color: "#fff"
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !VisionController.running
                            cursorShape: Qt.PointingHandCursor
                            onClicked: VisionController.commitRulesToEngine()
                        }
                    }
                }
            }
        }

        CameraView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
        }
    }
}

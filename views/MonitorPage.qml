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
            Layout.preferredHeight: 56
            color: "#CAD5E2"
            border.color: "#d1d5db"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                Repeater {
                    model: ["Face Detection", "Object Detection", "Motion Detection"]

                    Rectangle {
                        required property string modelData
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 32
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
        }

        CameraView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
        }
    }
}

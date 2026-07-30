/*
 * Author - Muhammed Suwaneh
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: root
    width: 1200
    height: 900
    visible: true
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"

    property string currentSelectedProcessor: "Face Detection"

    Rectangle {
        id: appFrame
        anchors.fill: parent
        color: "#f8fafc"
        radius: 12
        clip: true

        GridLayout {
            anchors.fill: parent
            rows: 3
            rowSpacing: 0
            columnSpacing: 0

            // TITLE BAR
            TitleBar {
                id: titleBar
                Layout.row: 0
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                z: 10
            }

            // CONTROLS BAR
            Rectangle {
                id: controlsBar
                Layout.row: 1
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: "#CAD5E2"
                border.color: "#d1d5db"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Rectangle {
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 32
                        radius: 8
                        color: VisionController.running ? "#FF637E" : "#155DFC"
                        border.color: "#cad5e2"

                        Text {
                            anchors.centerIn: parent
                            text: VisionController.running ?  "Stop Camera" : "Start Camera"
                            font.pixelSize: 12
                            color: "#fff"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {

                                if(!VisionController.running) VisionController.startCamera();
                                else VisionController.stopCamera();
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Repeater
                    {
                        model: [ "Face Detection", "Object Detection", "Motion Detection" ]

                        Rectangle {
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 32
                            radius: 8
                            color: root.currentSelectedProcessor === modelData ? "#74D4FF" : "#e5e7eb"
                            border.color: "#cad5e2"
                            opacity: mouse.containsMouse ? 0.85 : 1

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: 12
                                color: root.currentSelectedProcessor === modelData ?  "#fff" : "#1C69A8"
                            }

                            MouseArea {
                                id: mouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked:
                                {
                                    root.currentSelectedProcessor = modelData
                                    VisionController.setMode(modelData)
                                }
                            }
                        }
                    }
                }
            }

            // CAMERA View
            CameraView
            {
                id: cameraView
                Layout.row: 2
                Layout.topMargin: 10
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: parent.width - 25
                Layout.preferredHeight: parent.height - (controlsBar.height + titleBar.height)
            }
        }
    }
}

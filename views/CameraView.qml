/*
 * Author - Muhammed Suwaneh
*/

import QtQuick

Item {
    id: root

    Rectangle {
        id: container
        width: parent.width
        height: parent.height - 20
        radius: 20
        color: "#000"

        Rectangle {
            id: mask
            anchors.fill: parent
            radius: container.radius
            color: "transparent"
            clip: true

            Item {
                id: videoItem
                anchors.fill: parent
                anchors.margins: 10

                Image {
                    id: cameraView
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    cache: false
                    smooth: true
                    visible: VisionController.running
                    source: VisionController.running ? "image://camera/live" : ""

                    Connections {
                        target: camera
                        function onFrameCleared() {
                            cameraView.source = ""
                        }

                        function onFrameChanged() {
                            if (VisionController.running)
                                cameraView.source = "image://camera/live?" + Date.now()
                        }
                    }

                    Behavior on opacity {
                        NumberAnimation { duration: 250 }
                    }
                }

                RuleOverlay {
                    anchors.fill: parent
                }

                MouseArea {
                    id: drawArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    onClicked: function(mouse) {
                        forceActiveFocus()
                        VisionController.addDrawPoint(mouse.x, mouse.y, width, height)
                    }
                    onDoubleClicked: VisionController.finishDraw()
                    Keys.onEscapePressed: function(event) {
                        VisionController.cancelDraw()
                        event.accepted = true
                    }
                    Keys.onPressed: function(event) {
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            VisionController.finishDraw()
                            event.accepted = true
                        }
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            text: I18n.cameraOff
            color: "white"
            font.pixelSize: 22
            opacity: VisionController.running ? 0 : 1

            Behavior on opacity {
                NumberAnimation { duration: 250 }
            }
        }
    }
}

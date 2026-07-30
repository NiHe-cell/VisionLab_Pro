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

        // 🔹 Mask wrapper (important)
        Rectangle {
            id: mask
            anchors.fill: parent
            radius: container.radius
            color: "transparent"
            clip: true

            Image {
                id: cameraView
                anchors.fill: parent
                anchors.margins: 10
                fillMode: Image.PreserveAspectCrop
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
        }

        Text {
            anchors.centerIn: parent
            text: "Camera Off"
            color: "white"
            font.pixelSize: 22
            opacity: VisionController.running ? 0 : 1

            Behavior on opacity {
                NumberAnimation { duration: 250 }
            }
        }
    }
}

import QtQuick

Item {
    id: root

    readonly property var stats: VisionController.performanceModel

    function backendLabel() {
        const index = VisionController.uiBackend()
        if (index === 1)
            return I18n.backendCuda
        if (index === 2)
            return I18n.backendTensorRt
        return I18n.backendCpu
    }

    Rectangle {
        id: container
        anchors.fill: parent
        radius: Theme.radiusMd
        color: "#000000"
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
                opacity: VisionController.running ? 1 : 0

                Behavior on opacity {
                    enabled: !Theme.reducedMotion
                    NumberAnimation { duration: Theme.durationMed }
                }

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

            Row {
                z: 2
                spacing: Theme.spaceSm
                anchors.left: parent.left
                anchors.top: parent.top
                visible: VisionController.running

                Repeater {
                    model: [
                        stats.inferenceFps.toFixed(1) + " FPS",
                        "P95 " + stats.p95InferenceLatencyMs.toFixed(1) + " ms",
                        root.backendLabel()
                    ]

                    Rectangle {
                        id: hudChip
                        required property string modelData
                        height: 28
                        width: chipText.implicitWidth + Theme.spaceXl
                        radius: Theme.radiusSm
                        color: Theme.glassFill
                        border.color: Theme.glassBorder
                        border.width: 1

                        Text {
                            id: chipText
                            anchors.centerIn: parent
                            text: hudChip.modelData
                            color: Theme.foreground
                            font.pixelSize: Theme.caption
                            font.family: Theme.fontMono
                        }
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            text: I18n.cameraOff
            color: Theme.mutedForeground
            font.pixelSize: Theme.h2
            font.family: Theme.fontUi
            opacity: VisionController.running ? 0 : 1

            Behavior on opacity {
                enabled: !Theme.reducedMotion
                NumberAnimation { duration: Theme.durationMed }
            }
        }
    }
}

/*
 * Author - Muhammed Suwaneh
*/

import QtQuick
import QtQuick.Controls


Item {
    id: root
    clip: true
    width: parent.width
    height: 50

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#0F172B"
        id: titleBar
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 12
        color: "#0F172B"
    }

    Row
    {
        spacing: 5
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter

        Image {
            width: 20
            height: 20
            mipmap: true
            fillMode: Image.PreserveAspectCrop
            source: "../assets/logo.png"
        }

        Text {
            text: I18n.appTitle
            color: "white"
            font.pixelSize: 15
            font.bold: true
        }
    }

    MouseArea {
        anchors.left: parent.left
        anchors.right: controlButtons.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        onPressed: WindowController.startDrag(mouse.x, mouse.y)
        onDoubleClicked: WindowController.maximize()
    }

    Row {
        id: controlButtons
        spacing: 10

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            width: 88
            height: 22
            radius: 6
            color: VisionController.running ? "#FF637E" : "#155DFC"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: VisionController.running ? I18n.stopCamera : I18n.startCamera
                color: "white"
                font.pixelSize: 11
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!VisionController.running)
                        VisionController.startCamera()
                    else
                        VisionController.stopCamera()
                }
            }
        }

        Rectangle {
            width: 40
            height: 22
            radius: 6
            color: "#1E293B"
            border.color: "#334155"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: I18n.languageButton
                color: "white"
                font.pixelSize: 12
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: LocaleController.toggle()
            }
        }

        ControlButton { backgroundColor: "#FFD230"; onClicked: WindowController.minimize() }
        ControlButton { backgroundColor: "#05DF72"; onClicked: WindowController.maximize() }
        ControlButton
        {
            backgroundColor: "#F4320B";
            onClicked: {
                if(VisionController.running) VisionController.stopCamera();
                WindowController.close()
            }
        }
    }
}


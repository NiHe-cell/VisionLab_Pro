import QtQuick
import QtQuick.Layouts

Item {
    id: root
    clip: true

    Rectangle {
        anchors.fill: parent
        color: Theme.primary
    }

    RowLayout {
        spacing: Theme.spaceSm
        anchors.left: parent.left
        anchors.leftMargin: Theme.spaceLg
        anchors.verticalCenter: parent.verticalCenter

        Image {
            Layout.preferredWidth: Theme.icon
            Layout.preferredHeight: Theme.icon
            sourceSize.width: Theme.icon
            sourceSize.height: Theme.icon
            mipmap: true
            fillMode: Image.PreserveAspectFit
            source: "../assets/logo.png"
        }

        Text {
            text: I18n.appTitle
            color: Theme.foreground
            font.pixelSize: Theme.body
            font.family: Theme.fontUi
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
        spacing: Theme.spaceMd
        anchors.right: parent.right
        anchors.rightMargin: Theme.spaceMd
        anchors.verticalCenter: parent.verticalCenter

        ToolChip {
            text: VisionController.running ? I18n.stopCamera : I18n.startCamera
            primary: !VisionController.running
            danger: VisionController.running
            implicitWidth: Math.max(108, implicitContentWidth + Theme.spaceXl)
            onClicked: {
                if (!VisionController.running)
                    VisionController.startCamera()
                else
                    VisionController.stopCamera()
            }
        }

        ToolChip {
            text: I18n.languageButton
            implicitWidth: Theme.hit + Theme.spaceMd
            onClicked: LocaleController.toggle()
        }

        ControlButton {
            glyph: "\u2013"
            Accessible.name: I18n.windowMinimize
            onClicked: WindowController.minimize()
        }
        ControlButton {
            glyph: "\u25A1"
            Accessible.name: I18n.windowMaximize
            onClicked: WindowController.maximize()
        }
        ControlButton {
            glyph: "\u00D7"
            destructive: true
            Accessible.name: I18n.windowClose
            onClicked: {
                if (VisionController.running)
                    VisionController.stopCamera()
                WindowController.close()
            }
        }
    }
}

import QtQuick
import QtQuick.Controls

Item {
    ListView {
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        model: VisionController.eventFilterModel
        delegate: Text {
            required property string message
            required property string ruleId
            required property string label
            width: ListView.view.width
            text: message + "  " + ruleId + "  " + label
            color: "#0F172B"
            font.pixelSize: 13
            padding: 6
        }
    }
}

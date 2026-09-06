import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    function applyAll() {
        const okSettings = VisionController.applyUiSettings(
            backendBox.currentIndex,
            precisionBox.currentIndex,
            deviceBox.value,
            confidenceSlider.value,
            nmsSlider.value,
            trackingSwitch.checked)
        const settingsError = VisionController.lastSettingsError()
        const okRules = VisionController.applyUiRules()
        if (!okSettings)
            errorText.text = settingsError
        else if (!okRules)
            errorText.text = VisionController.lastSettingsError()
        else
            errorText.text = ""
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 24
        clip: true

        ColumnLayout {
            width: root.width - 48
            spacing: 12

            Text {
                text: I18n.navSettings
                font.pixelSize: 18
                font.bold: true
                color: "#0F172B"
            }

            GridLayout {
                columns: 2
                columnSpacing: 16
                rowSpacing: 8
                Layout.fillWidth: true

                Text { text: I18n.settingsBackend; color: "#334155"; font.pixelSize: 13 }
                ComboBox {
                    id: backendBox
                    Layout.preferredWidth: 200
                    model: [I18n.backendCpu, I18n.backendCuda, I18n.backendTensorRt]
                    currentIndex: VisionController.uiBackend()
                }

                Text { text: I18n.settingsPrecision; color: "#334155"; font.pixelSize: 13 }
                ComboBox {
                    id: precisionBox
                    Layout.preferredWidth: 200
                    model: [I18n.precisionFp32, I18n.precisionFp16]
                    currentIndex: Math.min(VisionController.uiPrecision(), 1)
                }

                Text { text: I18n.settingsDevice; color: "#334155"; font.pixelSize: 13 }
                SpinBox {
                    id: deviceBox
                    from: 0
                    to: 7
                    value: VisionController.uiDeviceId()
                }

                Text { text: I18n.settingsConfidence; color: "#334155"; font.pixelSize: 13 }
                RowLayout {
                    Slider {
                        id: confidenceSlider
                        from: 0
                        to: 1
                        stepSize: 0.01
                        value: VisionController.uiConfidence()
                        Layout.preferredWidth: 160
                    }
                    Text { text: confidenceSlider.value.toFixed(2); color: "#0F172B"; font.pixelSize: 12 }
                }

                Text { text: I18n.settingsNms; color: "#334155"; font.pixelSize: 13 }
                RowLayout {
                    Slider {
                        id: nmsSlider
                        from: 0
                        to: 1
                        stepSize: 0.01
                        value: VisionController.uiNms()
                        Layout.preferredWidth: 160
                    }
                    Text { text: nmsSlider.value.toFixed(2); color: "#0F172B"; font.pixelSize: 12 }
                }

                Text { text: I18n.settingsTracking; color: "#334155"; font.pixelSize: 13 }
                Switch {
                    id: trackingSwitch
                    checked: VisionController.uiTracking()
                }
            }

            Text {
                text: I18n.settingsPlugins
                font.pixelSize: 14
                font.bold: true
                color: "#0F172B"
            }

            ListView {
                id: pluginList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(72, count * 28)
                clip: true
                model: VisionController.pluginModel
                delegate: Text {
                    required property string pluginId
                    required property string name
                    required property string version
                    required property string modeLabel
                    width: pluginList.width
                    text: pluginId + "  " + name + "  " + version + "  " + modeLabel
                    color: "#334155"
                    font.pixelSize: 12
                }
            }

            Text {
                visible: VisionController.pluginModel.loadErrors().length > 0
                text: VisionController.pluginModel.loadErrors().join("\n")
                color: "#B45309"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: I18n.settingsRules
                font.pixelSize: 14
                font.bold: true
                color: "#0F172B"
            }

            ListView {
                id: ruleList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(72, count * 36)
                clip: true
                model: VisionController.ruleModel
                delegate: RowLayout {
                    required property int index
                    required property string ruleId
                    required property int kind
                    required property bool enabled
                    required property double loiterSeconds
                    width: ruleList.width
                    spacing: 8

                    Switch {
                        checked: enabled
                        enabled: !VisionController.running
                        onToggled: VisionController.ruleModel.setEnabled(index, checked)
                    }
                    Text {
                        text: ruleId + "  " + I18n.eventTypeLabel(kind)
                        color: "#0F172B"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        visible: kind === 2
                        enabled: !VisionController.running
                        from: 1
                        to: 3600
                        value: Math.max(1, Math.round(loiterSeconds))
                        onValueModified: VisionController.ruleModel.setLoiterSeconds(index, value)
                    }
                }
            }

            Button {
                text: I18n.applySettings
                enabled: !VisionController.running
                onClicked: root.applyAll()
            }

            Text {
                id: errorText
                color: "#B91C1C"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                text: VisionController.lastSettingsError
            }
        }
    }
}

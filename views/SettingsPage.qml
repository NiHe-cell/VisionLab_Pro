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
        anchors.margins: Theme.space2xl
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width - Theme.space2xl * 2
            spacing: Theme.spaceXl

            Text {
                text: I18n.navSettings
                font.pixelSize: Theme.h1
                font.family: Theme.fontUi
                font.bold: true
                color: Theme.foreground
            }

            Text {
                visible: VisionController.running
                text: I18n.settingsLockedWhileRunning
                color: Theme.mutedForeground
                font.pixelSize: Theme.caption
                font.family: Theme.fontUi
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            SettingsCard {
                title: I18n.settingsInference
                Layout.fillWidth: true

                GridLayout {
                    columns: 2
                    columnSpacing: Theme.spaceXl
                    rowSpacing: Theme.spaceMd
                    Layout.fillWidth: true

                    Text { text: I18n.settingsBackend; color: Theme.mutedForeground; font.pixelSize: Theme.body; font.family: Theme.fontUi }
                    ComboBox {
                        id: backendBox
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: Theme.hit
                        enabled: !VisionController.running
                        model: [I18n.backendCpu, I18n.backendCuda, I18n.backendTensorRt]
                        currentIndex: VisionController.uiBackend()
                    }

                    Text { text: I18n.settingsPrecision; color: Theme.mutedForeground; font.pixelSize: Theme.body; font.family: Theme.fontUi }
                    ComboBox {
                        id: precisionBox
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: Theme.hit
                        enabled: !VisionController.running
                        model: [I18n.precisionFp32, I18n.precisionFp16]
                        currentIndex: Math.min(VisionController.uiPrecision(), 1)
                    }

                    Text { text: I18n.settingsDevice; color: Theme.mutedForeground; font.pixelSize: Theme.body; font.family: Theme.fontUi }
                    SpinBox {
                        id: deviceBox
                        from: 0
                        to: 7
                        value: VisionController.uiDeviceId()
                        enabled: !VisionController.running
                        Layout.preferredHeight: Theme.hit
                    }

                    Text { text: I18n.settingsConfidence; color: Theme.mutedForeground; font.pixelSize: Theme.body; font.family: Theme.fontUi }
                    RowLayout {
                        Slider {
                            id: confidenceSlider
                            from: 0
                            to: 1
                            stepSize: 0.01
                            value: VisionController.uiConfidence()
                            enabled: !VisionController.running
                            Layout.preferredWidth: 160
                        }
                        Text {
                            text: confidenceSlider.value.toFixed(2)
                            color: Theme.foreground
                            font.pixelSize: Theme.caption
                            font.family: Theme.fontMono
                        }
                    }

                    Text { text: I18n.settingsNms; color: Theme.mutedForeground; font.pixelSize: Theme.body; font.family: Theme.fontUi }
                    RowLayout {
                        Slider {
                            id: nmsSlider
                            from: 0
                            to: 1
                            stepSize: 0.01
                            value: VisionController.uiNms()
                            enabled: !VisionController.running
                            Layout.preferredWidth: 160
                        }
                        Text {
                            text: nmsSlider.value.toFixed(2)
                            color: Theme.foreground
                            font.pixelSize: Theme.caption
                            font.family: Theme.fontMono
                        }
                    }

                    Text { text: I18n.settingsTracking; color: Theme.mutedForeground; font.pixelSize: Theme.body; font.family: Theme.fontUi }
                    Switch {
                        id: trackingSwitch
                        checked: VisionController.uiTracking()
                        enabled: !VisionController.running
                    }
                }
            }

            SettingsCard {
                title: I18n.settingsPlugins
                Layout.fillWidth: true

                ListView {
                    id: pluginList
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(Theme.hit * 2, count * 40)
                    clip: true
                    interactive: false
                    model: VisionController.pluginModel
                    delegate: Rectangle {
                        required property string pluginId
                        required property string name
                        required property string version
                        required property string modeLabel
                        width: pluginList.width
                        height: 36
                        color: "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            text: pluginId + "  " + name + "  " + version + "  " + modeLabel
                            color: Theme.foreground
                            font.pixelSize: Theme.caption
                            font.family: Theme.fontUi
                            elide: Text.ElideRight
                        }
                    }
                }

                Text {
                    visible: VisionController.pluginModel.loadErrors().length > 0
                    text: VisionController.pluginModel.loadErrors().join("\n")
                    color: "#F59E0B"
                    font.pixelSize: Theme.caption
                    font.family: Theme.fontUi
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            SettingsCard {
                title: I18n.settingsRules
                Layout.fillWidth: true

                ListView {
                    id: ruleList
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(Theme.hit * 2, count * 40)
                    clip: true
                    interactive: false
                    model: VisionController.ruleModel
                    delegate: RowLayout {
                        required property int index
                        required property string ruleId
                        required property int kind
                        required property bool enabled
                        required property double loiterSeconds
                        width: ruleList.width
                        height: Theme.hit
                        spacing: Theme.spaceMd

                        Switch {
                            checked: enabled
                            enabled: !VisionController.running
                            onToggled: VisionController.ruleModel.setEnabled(index, checked)
                        }
                        Text {
                            text: ruleId + "  " + I18n.eventTypeLabel(kind)
                            color: Theme.foreground
                            font.pixelSize: Theme.caption
                            font.family: Theme.fontUi
                            Layout.fillWidth: true
                            elide: Text.ElideRight
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
            }

            ToolChip {
                text: I18n.applySettings
                primary: true
                enabled: !VisionController.running
                onClicked: root.applyAll()
            }

            Text {
                id: errorText
                color: Theme.destructive
                font.pixelSize: Theme.caption
                font.family: Theme.fontUi
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                text: VisionController.lastSettingsError
            }
        }
    }
}

pragma Singleton
import QtQuick

QtObject {
    id: root

    readonly property bool zh: LocaleController.chinese

    readonly property string appTitle: zh ? "Vision Lab" : "Vision Lab"
    readonly property string startCamera: zh ? "开启摄像头" : "Start Camera"
    readonly property string stopCamera: zh ? "停止摄像头" : "Stop Camera"
    readonly property string faceDetection: zh ? "人脸检测" : "Face Detection"
    readonly property string objectDetection: zh ? "目标检测" : "Object Detection"
    readonly property string motionDetection: zh ? "运动检测" : "Motion Detection"
    readonly property string cameraOff: zh ? "摄像头未开启" : "Camera Off"
    readonly property string languageButton: zh ? "EN" : "中"
    readonly property string navMonitor: zh ? "监控" : "Monitor"
    readonly property string navEvents: zh ? "事件" : "Events"
    readonly property string navPerformance: zh ? "性能" : "Performance"
    readonly property string navSettings: zh ? "设置" : "Settings"
    readonly property string settingsPlaceholder: zh ? "设置页将在后续任务接入" : "Settings arrive in a later task"

    function modeLabel(key) {
        if (key === "Face Detection")
            return faceDetection
        if (key === "Object Detection")
            return objectDetection
        if (key === "Motion Detection")
            return motionDetection
        return key
    }
}

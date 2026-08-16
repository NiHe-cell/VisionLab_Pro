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

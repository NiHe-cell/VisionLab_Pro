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
    readonly property string toolRoi: zh ? "ROI" : "ROI"
    readonly property string toolLine: zh ? "越线" : "Line"
    readonly property string toolLoiter: zh ? "逗留" : "Loiter"
    readonly property string toolCount: zh ? "计数" : "Count"
    readonly property string applyRules: zh ? "应用规则" : "Apply Rules"
    readonly property string eventAll: zh ? "全部" : "All"
    readonly property string eventRoi: zh ? "ROI" : "ROI"
    readonly property string eventLine: zh ? "越线" : "Line"
    readonly property string eventLoiter: zh ? "逗留" : "Loiter"
    readonly property string eventCount: zh ? "计数" : "Count"
    readonly property string colTime: zh ? "时间" : "Time"
    readonly property string colType: zh ? "类型" : "Type"
    readonly property string colRule: zh ? "规则" : "Rule"
    readonly property string colTrack: zh ? "轨迹" : "Track"
    readonly property string colLabel: zh ? "标签" : "Label"
    readonly property string colMessage: zh ? "消息" : "Message"
    readonly property string eventsEmpty: zh ? "暂无事件" : "No events"
    readonly property string statCaptureFps: zh ? "采集 FPS" : "Capture FPS"
    readonly property string statInferenceFps: zh ? "推理 FPS" : "Inference FPS"
    readonly property string statP50: zh ? "推理 P50 (ms)" : "Inference P50 (ms)"
    readonly property string statP95: zh ? "推理 P95 (ms)" : "Inference P95 (ms)"
    readonly property string statE2e: zh ? "端到端 (ms)" : "End-to-end (ms)"
    readonly property string statQueue: zh ? "队列深度" : "Queue depth"
    readonly property string statDropped: zh ? "丢帧" : "Dropped frames"
    readonly property string statTracks: zh ? "活动轨迹" : "Active tracks"
    readonly property string statEvents: zh ? "已发事件" : "Events emitted"
    readonly property string statRuleLatency: zh ? "规则均值 (ms)" : "Rule latency (ms)"
    readonly property string statEnabledRules: zh ? "启用规则" : "Enabled rules"
    readonly property string perfSnapshotHint: zh ? "以上为 250ms 实时快照，不是基准测试结果。" : "Live 250ms snapshot, not a benchmark file."
    readonly property string settingsBackend: zh ? "推理后端" : "Backend"
    readonly property string settingsPrecision: zh ? "精度" : "Precision"
    readonly property string settingsDevice: zh ? "设备" : "Device"
    readonly property string settingsConfidence: zh ? "置信度" : "Confidence"
    readonly property string settingsNms: zh ? "NMS" : "NMS"
    readonly property string settingsTracking: zh ? "跟踪" : "Tracking"
    readonly property string settingsPlugins: zh ? "已加载插件" : "Loaded plugins"
    readonly property string settingsRules: zh ? "规则" : "Rules"
    readonly property string applySettings: zh ? "应用" : "Apply"
    readonly property string backendCpu: "ONNX CPU"
    readonly property string backendCuda: "ONNX CUDA"
    readonly property string backendTensorRt: "TensorRT"
    readonly property string precisionFp32: "FP32"
    readonly property string precisionFp16: "FP16"

    function modeLabel(key) {
        if (key === "Face Detection")
            return faceDetection
        if (key === "Object Detection")
            return objectDetection
        if (key === "Motion Detection")
            return motionDetection
        return key
    }

    function eventTypeLabel(type) {
        if (type === 0)
            return eventRoi
        if (type === 1)
            return eventLine
        if (type === 2)
            return eventLoiter
        if (type === 3)
            return eventCount
        return String(type)
    }
}

# YOLOv4-tiny ONNX 模型约定

Object 模式不再读取 Darknet `.cfg` / `.weights`。把转换好的
`yolov4-tiny.onnx` 和 `coco.names` 放在本目录（应用运行时从
`VisionLab/models/` 加载）。

不要在 CMake 或推理引擎里下载模型。

## 期望的 ONNX 布局

- 输入：`1×3×320×320`，`float32`，NCHW，RGB，数值范围 `[0, 1]`
- 预处理：stretch 到 320×320（**不是** letterbox），BGR→RGB，÷255
- 每个输出 squeeze 后为 `[N, 85]`，行布局
  `[cx, cy, w, h, objectness, 80 classes]`
- 框坐标是相对输入尺寸的归一化值；解码时乘以**原图**宽高
  （与 `yolo::decodeDetections` 一致）

Opset 与导出工具不限，只要满足上述张量语义。动态维（shape 含 `-1`）本阶段不支持。

## 转换

用现有 `yolov4-tiny.cfg` / `.weights` 自行导出 ONNX，或放置等价的官方转换结果。
工厂只认文件名 `yolov4-tiny.onnx`；文件缺失时 `isReady()` 为 false，管线仍出画面。

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

## 推理后端选择

不改代码、不加设置页。启动前设置环境变量（非法值会告警并回退到 CPU / fp32 / 0；**GPU 初始化失败不会改走 CPU**）：

- `VISIONLAB_INFERENCE_BACKEND`：`onnx-cpu`（默认）、`onnx-cuda`、`tensorrt`
- `VISIONLAB_INFERENCE_PRECISION`：`fp32`（默认）、`fp16`（仅 TensorRT；ORT CPU/CUDA 会 initialize 失败）
- `VISIONLAB_INFERENCE_DEVICE`：CUDA / TensorRT 设备号，默认 `0`

Object 模式选了 GPU 但引擎未就绪时画面仍在，只是没有检测框。不要把它当成摄像头故障；看日志里的 `lastError`。

## 推理基准对比

`bench_onnx_cpu`（CMake 别名 `bench_inference`）经 `createInferenceEngine` 跑**一种**后端。
对比 ORT CPU / ORT CUDA / TensorRT FP32 / TensorRT FP16 时请启动四次进程，不要在同一进程连跑（避免设备内存互相干扰）。

```
bench_onnx_cpu --backend onnx-cpu --model <path.onnx>
bench_onnx_cpu --backend onnx-cuda --model <path.onnx>
bench_onnx_cpu --backend tensorrt --precision fp32 --model <path.onnx>
bench_onnx_cpu --backend tensorrt --precision fp16 --model <path.onnx>
```

TensorRT 的输入高宽必须与 ONNX 静态 `1×3×H×W` 一致（默认 320，对应 `yolov4-tiny`）。identity 夹具需加 `--input-width 2 --input-height 2`。

成功时 stdout 含 `mean_ms` / `p50_ms` / `p95_ms` / `p99_ms` / `throughput_fps`。
GPU 后端额外打印 `gpu_mem_mb`：`cudaMemGetInfo` 的 `(total-free)`，单位 MiB，取自定时循环结束后的设备占用；查询失败则省略该行。
缺模型、未知 `--backend`、引擎 initialize 失败：非 0 退出且不打印 `mean_ms`。不把延迟数字写进本文件。

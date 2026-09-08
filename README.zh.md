# VisionLab Pro

![CI](https://github.com/NiHe-cell/VisionLab_Pro/actions/workflows/ci.yml/badge.svg)

实时视频分析与边缘推理，在上游 VisionLab 摄像头演示上做的架构向二次开发。C++20、Qt 6 QML、OpenCV、ONNX Runtime，可选 TensorRT 10。桌面壳是暗色 Night Ops 操控台（`views/Theme.qml`）；采集、推理、跟踪、规则不在 Qt GUI 线程上跑。

English: [README.md](README.md)

克隆本仓库：

```
git clone https://github.com/NiHe-cell/VisionLab_Pro.git
```

上游原演示：[Muhammedsuwaneh/vision-lab](https://github.com/Muhammedsuwaneh/vision-lab)。

本说明描述**当前这棵树**。CI 徽章在 Actions 变绿之前不代表已经通过。文中没有 FPS / 延迟承诺；测完再填 [`docs/performance.md`](docs/performance.md)。

## 概述

采集、推理、跟踪、规则、叠加绘制、SQLite 落库是分开的模块。Qt GUI 线程不调用 `IDetector::detect` 或 `IInferenceEngine::infer`。检测器返回 `Detection` 结构，不在 `cv::Mat` 上画框。帧队列反压是 `BoundedQueue` + `DropOldest`。

## 功能

| 功能 | 位置 |
|---|---|
| 有界帧队列，DropOldest | `core/BoundedQueue.h`，`CaptureWorker` |
| 推理后端共用一个端口 | `IInferenceEngine`，`createInferenceEngine` |
| 运行时检测插件 | `PluginManager`，`vision.motion` / `vision.face` / `vision.yolo` |
| 多目标跟踪 | `ByteTrackTracker`（`ITracker`） |
| 四条分析规则 | ROI、越线、逗留、计数（`IRule`） |
| SQLite 事件写入 | `EventWriter` + `SqliteEventRepository`（`prune(10000)`） |
| 四页 Night Ops QML | 监控、事件、性能、设置（`Theme`、图标栏） |

不是本项目：C++17 作为语言标准（实际是 C++20）、检测器自己画框、热路径上的 detached `std::thread`（worker 是 `std::jthread`）。

## 界面

Night Ops 暗色壳。令牌在 `views/Theme.qml` 和 `design-system/visionlab-pro/MASTER.md`。QML 仍然只跟 `VisionController` 说话。

- 监控：实时画面；停机时画规则；运行时 HUD
- 事件：类型芯片 + 历史表
- 性能：KPI 卡，来自 250 ms `PipelineStats` 快照，**不是**基准测试
- 设置：推理 / 插件 / 规则分组；仅停机 Apply

契约：[`docs/ui/qml-boundary.md`](docs/ui/qml-boundary.md)。视觉规格：[`docs/superpowers/specs/2026-09-08-night-ops-ui-design.md`](docs/superpowers/specs/2026-09-08-night-ops-ui-design.md)。

`screenshots/*.png` 是上游单页界面的历史截图，不是当前 Night Ops 四页壳，也不是测得的 FPS。

## 构建

[`docs/build.md`](docs/build.md)。需要 C++20、Qt 6.8+（Sql、Quick、Qml、Svg、QuickControls2）、OpenCV MSVC 包、ORT CPU zip 1.17+。

```
cmake --preset dev-debug
cmake --build --preset dev-debug
```

## 测试

```
ctest --test-dir build/dev-debug -L cpu --output-on-failure
```

标签与跳过策略：[`docs/testing.md`](docs/testing.md)。CPU CI：`.github/workflows/ci.yml`（不含 CUDA）。

## 许可

在 [Muhammed Suwaneh 的 VisionLab](https://github.com/Muhammedsuwaneh/vision-lab)（MIT）上的二次架构。原版权仍在 `LICENSE`。VisionLab Pro 的增量同样 MIT。

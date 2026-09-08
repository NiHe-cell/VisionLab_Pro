# VisionLab Pro

![CI](https://github.com/NiHe-cell/VisionLab_Pro/actions/workflows/ci.yml/badge.svg)

Real-time video analytics and edge inference, rebuilt from the upstream
VisionLab camera demo. C++20, Qt 6 QML, OpenCV, ONNX Runtime, optional
TensorRT 10. The desktop shell is a dark Night Ops command center
(`views/Theme.qml`); capture, inference, tracking, and rules stay off the
GUI thread.

中文说明：[README.zh.md](README.zh.md)

Clone this fork:

```
git clone https://github.com/NiHe-cell/VisionLab_Pro.git
```

Upstream (original demo): [Muhammedsuwaneh/vision-lab](https://github.com/Muhammedsuwaneh/vision-lab).

This README describes **this** tree. It does not claim a GitHub Actions run
is green until Actions shows a pass. It does not contain FPS or latency
guarantees; fill `docs/performance.md` after you measure.

## Overview

Capture, inference, tracking, rules, overlay, and SQLite persistence are
separate modules. The Qt GUI thread does not run `IDetector::detect` or
`IInferenceEngine::infer`. Detectors return `Detection` structs; they do not
draw on `cv::Mat`. Frame backpressure is `BoundedQueue` with `DropOldest`.

## Features

| Feature | Where |
|---|---|
| Bounded frame queue, DropOldest | `core/BoundedQueue.h`, `CaptureWorker` |
| Inference backends behind one port | `IInferenceEngine`, `createInferenceEngine` |
| Runtime detector plugins | `PluginManager`, `vision.motion` / `vision.face` / `vision.yolo` |
| Multi-object tracking | `ByteTrackTracker` (`ITracker`) |
| Four analytics rules | ROI, line crossing, loitering, counting (`IRule`) |
| SQLite event writer | `EventWriter` + `SqliteEventRepository` (`prune(10000)`) |
| Four-page Night Ops QML UI | Monitor, Events, Performance, Settings (`Theme`, icon rail) |

Not this project: C++17 as the language standard (it is C++20), detectors
painting boxes, detached `std::thread` on the hot path (workers are
`std::jthread`).

## Demo

`screenshots/MainWindow.png`, `FaceDetection.png`, `ObjectDetection.png`, and
`MotionDetection.png` are **historical** captures of the upstream single-view
UI. The current shell is four Night Ops pages (`views/Main.qml`, dark
`Theme` tokens, 64 px icon rail). Do not treat those PNGs as the live UI,
the Performance page, or a measured FPS.

## Architecture

Diagrams: [`docs/architecture.md`](docs/architecture.md).

QML → `VisionController` → `CameraManager` → `PluginManager` /
`VisionPipeline` / `EventWriter`. Source code wins if a diagram drifts.

## Real-time pipeline

`CaptureWorker` (`jthread`) reads `IVideoSource`, stamps `FramePacket`, pushes
`BoundedQueue<FramePacket>` (default capacity 2, DropOldest).
`InferenceWorker` (`jthread`) pops, detects, optionally tracks, evaluates
rules, renders, publishes `LatestResult<PresentedFrame>`. The GUI copies a
`QImage` via `Qt::QueuedConnection`. There is no `RuleWorker`.

## Inference

Object mode: `YoloDetector` → `IInferenceEngine` → ORT CPU, ORT CUDA, or
TensorRT 10. Face: OpenCV DNN Caffe. Motion: MOG2. Place `yolov4-tiny.onnx`
next to `coco.names` under `models/` (not git-tracked). See
`models/README.md`.

## GPU

Optional. `VISIONLAB_ENABLE_TENSORRT=ON` plus CUDA + TensorRT 10 headers.
Runtime: `VISIONLAB_INFERENCE_BACKEND=onnx-cuda` or `tensorrt`. Failed GPU
initialize does not fall back to CPU. INT8 is out of scope.

## Plugins

`IVisionPlugin` via `QPluginLoader`. V1 does not unload plugins. Inference
backends are not plugins. Contract: `docs/plugins/detector-plugins.md`.

## Tracking

`ByteTrackTracker` on the inference thread after `detect`. Mode changes reset
the tracker and the rule engine.

## Analytics

Foot-point geometry, transition events, `RuleSpec` applied only while
stopped. Default production `RuleEngine` is empty until the UI injects specs.
Contract: `docs/analytics/rule-engine.md`.

## UI

Night Ops dark shell. Tokens live in `views/Theme.qml` and
`design-system/visionlab-pro/MASTER.md`. QML still talks only to
`VisionController`.

- Monitor: live frame, mode + rule tools when stopped, HUD chips while running
- Events: type chips + history table
- Performance: KPI cards from a 250 ms `PipelineStats` snapshot — **not** a
  benchmark
- Settings: Inference / Plugins / Rules cards; apply when stopped

Boundary: `docs/ui/qml-boundary.md`. Visual spec:
`docs/superpowers/specs/2026-09-08-night-ops-ui-design.md`.

## Performance

Templates with `[NOT MEASURED]` cells: [`docs/performance.md`](docs/performance.md).
Interview talking points: [`docs/interview-notes.md`](docs/interview-notes.md).
Resume placeholders: [`docs/resume-evidence.md`](docs/resume-evidence.md).

## Project structure

```
analytics/     IRule, RuleEngine, RuleSpec
benchmarks/    bench_onnx_cpu, bench_pipeline, bench_tracker
cmake/         FindOnnxRuntime, FindTensorRT
controllers/   VisionController
core/          BoundedQueue, FramePacket, Detection, Track, VisionEvent
detectors/     IDetector + Face / Motion / YOLO (no drawing)
design-system/ Night Ops color/type tokens (MASTER + page overrides)
docs/          architecture, build, testing, benchmarks, …
inference/     IInferenceEngine implementations
models/        Qt list models and detector files (mixed on purpose for V1)
pipeline/      VisionPipeline, workers, EventLog
plugin/        PluginManager, IVisionPlugin
plugins/       vision_dummy / motion / face / yolo
rendering/     DetectionRenderer, TrackRenderer, Letterbox
storage/       EventWriter, SQLite
tests/         CTest cpu / gpu labels
tracking/      ByteTrackTracker
utilities/     CameraManager
video/         IVideoSource, CameraSource, FakeVideoSource
views/         QML pages, `Theme` singleton, Night Ops controls
```

## Build

[`docs/build.md`](docs/build.md) — presets, `OpenCV_DIR`, `OnnxRuntime_DIR`,
no machine-absolute paths in CMake.

```
cmake --preset dev-debug
cmake --build --preset dev-debug
```

Requires C++20, Qt 6.8+ (Sql, Quick, Qml, Svg, QuickControls2), OpenCV
MSVC pack, ORT CPU zip 1.17+. MinGW cannot link the official OpenCV `vc16`
tree.

## Configuration

| Variable | Role |
|---|---|
| `VISIONLAB_INFERENCE_BACKEND` | `onnx-cpu` (default), `onnx-cuda`, `tensorrt` |
| `VISIONLAB_INFERENCE_PRECISION` | `fp32` (default), `fp16` (TensorRT) |
| `VISIONLAB_INFERENCE_DEVICE` | GPU index, default `0` |
| `VISIONLAB_PLUGIN_DIR` | Plugin scan directory; empty → `<app>/plugins` |

CMake: `VISIONLAB_ENABLE_TENSORRT` (default OFF), `VISIONLAB_ENABLE_WARNINGS`
(default OFF, not `-Werror`).

## Testing

```
ctest --test-dir build/dev-debug -L cpu --output-on-failure
```

Labels and skip policy: [`docs/testing.md`](docs/testing.md). CPU CI:
`.github/workflows/ci.yml` (no CUDA).

## Benchmark

Commands and stdout keys: [`docs/benchmarks.md`](docs/benchmarks.md).
`ctest` bench targets check keys and exit codes, not millisecond gates.

## Roadmap

**V1.0** is the architecture through Phase 9: CPU pipeline, plugins, ByteTrack,
rules, four-page UI, SQLite, CPU CTest label, benches, docs, attribution.
Checklist: [`docs/release-checklist.md`](docs/release-checklist.md). Review:
[`docs/architecture-review.md`](docs/architecture-review.md).

**Future (not V1):** INT8, qmltestrunner, plugin unload / hot-reload, rule
mutate while running, larger EventLog, default sanitizers, refreshed
screenshots of the Night Ops shell, renaming `models/` so Qt models and
weights are not mixed.

## Upstream / Attribution

VisionLab Pro is a secondary architecture on
[Muhammed Suwaneh's VisionLab](https://github.com/Muhammedsuwaneh/vision-lab)
(MIT). Original copyright remains in `LICENSE`. Additional VisionLab Pro work
is also MIT; see the extra copyright line in that file.

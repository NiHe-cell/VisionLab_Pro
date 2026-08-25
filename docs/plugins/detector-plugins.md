# Detector plugins (VisionLab Pro Phase 5)

This document is the implementation contract for `IVisionPlugin` and
`PluginManager`. T02–T08 must follow it. Changing a decision means editing
this file first.

V1 does **not** support plugin hot-reload, unloading while detectors exist,
a QML plugin picker, replacing `DetectionMode` in `VisionPipeline`, or
treating inference backends as plugins.

## Locked versions and APIs

- Qt 6 `QPluginLoader` + `Q_PLUGIN_METADATA`.
- IID: `com.visionlab.IVisionPlugin/1.0` (`VisionLab_IVisionPlugin_iid`).
- `PluginMetadata.interfaceVersion` must be **1**. Other values are rejected.
- Public headers of `IVisionPlugin` must not include `QPluginLoader`, QML
  types, ONNX Runtime, or TensorRT.
- `visionlab_core` must not link Qt. Plugin types live in `plugin/`.

## Plugin ids and modes

| id | CMake target | `metadata.mode` | Production pipeline |
|---|---|---|---|
| `vision.dummy` | `vision_dummy` | `nullopt` | No |
| `vision.motion` | `vision_motion` | `Motion` | Yes (T05+) |
| `vision.face` | `vision_face` | `Face` | Yes (T06+) |
| `vision.yolo` | `vision_yolo` | `Object` | Yes (T07+) |

Dummy exists to prove runtime discovery. It is not shown in QML and is not
inserted into `map<DetectionMode, unique_ptr<IDetector>>`.

Capabilities for V1: a single entry `"detect"`.

## DetectorCreateRequest

- `modelDir`: directory containing model files (no filename).
- `backend` / `precision` / `deviceId`: forwarded only by `vision.yolo`
  into `ModelConfig` and `createInferenceEngine`. Other plugins ignore them.
- Defaults: ONNX Runtime CPU, FP32, device 0.

YOLO plugins must not `new` an ORT session or TensorRT engine directly.

## Discovery

- Environment: `VISIONLAB_PLUGIN_DIR`. If unset or empty, scan
  `<applicationDir>/plugins`.
- Scan is **not recursive**.
- Only native libraries are considered (`*.dll` / `*.so` / `*.dylib`).
  `.pdb` / `.lib` / other sidecar files are ignored, not errors.
- A file with a library extension that is not a compatible plugin is
  skipped; `PluginManager::errors()` records the path and reason.
- Duplicate `id`: keep the first loaded plugin; record `duplicate` for
  later files.

## Lifetime

- `PluginManager` owns every `QPluginLoader` until its destructor.
- V1 has no `unload()`. Plugins stay loaded until process exit.
- Every `IDetector` created by a plugin must be destroyed **before**
  `PluginManager`. `CameraManager` must declare `PluginManager` as a
  member **before** `VisionPipeline` so destructors run in that order.
- `scan` / `createDetector` are not thread-safe. Call them on the GUI
  thread before `VisionPipeline::start()`. Do not scan while inference
  is running.

## ABI

Plugins and the host must be built with:

- the same Qt major version
- the same MSVC toolset (Windows)
- the same `visionlab_core` headers / layout

There is no binary compatibility promise across Qt major versions.
There is no safe unload of a plugin that has outstanding `IDetector`
instances.

## Failure policy

- Missing plugin directory: `scan` succeeds with empty metadata and no
  errors.
- Empty directory: same.
- Load / IID / `interfaceVersion` / empty id failures: skip the file,
  append to `errors()`, do not abort.
- Unknown id or `createDetector` returning `nullptr`: return `nullptr`
  and append to `errors()`. The application must keep running.
- Do not fall back to `DetectorFactory` after T08 removes it.

## Unchanged code (Phase 5 remainder)

`IDetector`, `YoloPreprocess`, `YoloDecoding`, `VisionPipeline` thread
model, and `IInferenceEngine` stay as they are. Tracking and rules are
out of scope.

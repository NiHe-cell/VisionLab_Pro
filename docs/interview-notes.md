# Interview notes (tied to this repository)

Ten talking points. Each points at files in **this** tree. No latency or FPS
claims. If a number is needed, fill `docs/performance.md` first.

## 1. Bounded queues

Real-time stages must not grow without bound when inference is slower than
capture. `core/BoundedQueue.h` is a mutex + `condition_variable` queue with a
fixed capacity (constructor rejects `0`). Capture → inference uses
`BoundedQueue<FramePacket>`. `EventWriter` uses a second bounded queue of
commands (capacity 1024). `EventLog` is a third bounded mailbox (256).

## 2. DropOldest

`OverflowPolicy::DropOldest` drops the front item, counts `dropped`, then
pushes. Capture must not block the camera loop waiting for inference.
`CaptureWorker` pushes with that policy. `tests/PipelineOverloadTest.cpp`
(`pipelineoverload`) is the behavioral test. `bench_pipeline` can print
`dropped` / `peak_queue_depth`; `pipelinebench` does **not** assert
`dropped > 0` (fast machines flake).

## 3. Shutdown / lifetime

`VisionPipeline::joinWorkers` (`pipeline/VisionPipeline.cpp`): clear
`m_running`, `request_stop` on both `jthread`s, `IVideoSource::close`,
`BoundedQueue::close`, join capture then inference, reset workers and queue.
`CameraManager::~CameraManager` stops the pipeline **then** `EventWriter::stop`
(close command queue, join, close SQLite). Plugins: `PluginManager` is a
member **before** `m_pipeline` so `IDetector` instances die before
`QPluginLoader` (`utilities/CameraManager.h`). No detached `std::thread`.

## 4. Inference abstraction

Detectors must not construct ORT / TensorRT types. `IInferenceEngine`
(`inference/IInferenceEngine.h`) loads, infers, reports `backendId` /
`lastError`. `createInferenceEngine` (`inference/InferenceEngineFactory.cpp`)
returns `OnnxRuntimeEngine`, `OnnxRuntimeCudaEngine`, `TensorRTEngine`, or
`UnavailableInferenceEngine`. Public headers do not include NvInfer. GPU
init failure does not silently switch to CPU (`docs/build.md`).

## 5. TensorRT

Compiled only with `VISIONLAB_ENABLE_TENSORRT=ON`. TensorRT **10**, named I/O,
`enqueueV3`, static `1×3×H×W`. No INT8, no dynamic shapes in V1. Engine cache
file name (`docs/inference/tensorrt-engine.md` §14):

```
<stem>-s<size>-t<mtime_utc_sec>-g<gpu_slug>-v<trt_version>-p<fp32|fp16>-<1x3xHxW>.engine
```

Directory: `<parent of onnx>/.trt-cache/`. Dirty cache is rebuilt or rejected,
never executed.

## 6. Plugins

`PluginManager` + `IVisionPlugin` (`docs/plugins/detector-plugins.md`).
Ids: `vision.dummy`, `vision.motion`, `vision.face`, `vision.yolo`. V1 has
**no** `unload()`. Scan / `createDetector` are not thread-safe; GUI thread,
before `VisionPipeline::start()`. Inference backends are **not** plugins.

## 7. Detection vs track

`IDetector::detect` returns `Detection` boxes for the current frame
(`detectors/IDetector.h`). `ITracker::update` assigns identities over time
(`tracking/ITracker.h`). Production tracker is `ByteTrackTracker`. Overlay:
if `presented.tracks` is non-empty, `TrackRenderer`; else `DetectionRenderer`
(`pipeline/InferenceWorker.cpp`). Detectors do not `cv::rectangle` on the
input `cv::Mat`.

## 8. Rules

Four `IRule` types: ROI intrusion, line crossing, loitering, counting
(`docs/analytics/rule-engine.md`). Geometry uses the **foot point**
`(cx, box.y + height)`, including the polygon boundary. Events are
transitions, not per-frame spam. `RuleEngine::evaluate` is not thread-safe;
it runs only on the inference `jthread`. `CameraManager::applyRuleSpecs`
returns false while running (`utilities/CameraManager.cpp`). Default
production engine is **empty** until the UI injects `RuleSpec`.

## 9. Performance method

The Performance QML page is a **250 ms** `PipelineStats` snapshot
(`docs/ui/qml-boundary.md`). It is not a benchmark. Compare backends with
separate `bench_onnx_cpu` processes. Overload / soak: `bench_pipeline`.
Tracker-only: `bench_tracker`. Tables live in `docs/performance.md` and start
as `[NOT MEASURED]`.

## 10. Pitfalls that exist in this repo

- **System32 `onnxruntime.dll`:** Windows may load an old copy from System32
  before PATH. `CMakeLists.txt` and `tests/CMakeLists.txt` `POST_BUILD`-copy
  the package DLL next to the executable.
- **`PreserveAspectCrop` vs frame coordinates:** a cropped image cannot invert
  item clicks to frame pixels. `CameraView.qml` uses `Image.PreserveAspectFit`;
  mapping is `rendering/Letterbox.h` (`docs/ui/qml-boundary.md`). The title-bar
  logo still uses `PreserveAspectCrop` (not a video plane).
- **EventLog vs `latest()->events`:** `LatestResult<PresentedFrame>` is
  newest-wins, so intermediate `PresentedFrame.events` never reach the GUI.
  `EventLog` (256) plus `EventWriter` from `notifyFrame` persist what still
  sits in the log. If the GUI stalls, DropOldest on EventLog drops events
  before SQLite (`docs/storage/events.md`).

# Architecture (VisionLab Pro)

Five diagrams of the **current** pipeline. If a diagram disagrees with source,
**source wins**. There is no dedicated `RuleWorker` thread. Production
`CameraManager::assembleFromPlugins` injects an empty `RuleEngine`; the UI
pushes `RuleSpec` lists only while the pipeline is **stopped**.

## 1. Overall

QML talks only to `VisionController`. `CameraManager` owns discovery, the
pipeline, and persistence.

```mermaid
flowchart TB
  subgraph qml [QML GUI thread]
    Main["Main.qml"]
    Monitor["MonitorPage"]
    Events["EventsPage"]
    Perf["PerformancePage"]
    Settings["SettingsPage"]
    Main --> Monitor
    Main --> Events
    Main --> Perf
    Main --> Settings
  end

  VC["VisionController"]
  CM["CameraManager"]
  PM["PluginManager"]
  VP["VisionPipeline"]
  EW["EventWriter"]
  Repo["SqliteEventRepository"]

  Main --> VC
  VC --> CM
  CM --> PM
  CM --> VP
  CM --> EW
  EW --> Repo
```

`VisionController` and the `*Model` types stay on the GUI thread
(`docs/ui/qml-boundary.md`). Core / pipeline / detectors do not include QML.

## 2. Threads

Capture, inference, and SQLite run on `std::jthread`. Frames use a bounded
queue. The GUI is notified with `Qt::QueuedConnection`.

```mermaid
flowchart LR
  subgraph gui [GUI thread]
    VC["VisionController"]
    CM["CameraManager"]
    Models["DetectionModel / TrackModel / EventModel / PerformanceModel"]
  end

  subgraph cap [Capture jthread]
    CW["CaptureWorker"]
    Src["IVideoSource"]
  end

  subgraph inf [Inference jthread]
    IW["InferenceWorker"]
  end

  subgraph wr [EventWriter jthread]
    EW["EventWriter"]
  end

  Q["BoundedQueue of FramePacket\nOverflowPolicy::DropOldest"]
  LR["LatestResult of PresentedFrame"]
  EL["EventLog capacity 256 DropOldest"]

  Src --> CW --> Q --> IW
  IW --> LR
  IW --> EL
  IW -->|"invokeMethod QueuedConnection notifyFrame"| CM
  CM --> Models
  CM -->|"enqueue from EventLog snapshot"| EW
```

Shutdown in `CameraManager::~CameraManager`: `VisionPipeline::stop()` then
`EventWriter::stop()`. `VisionPipeline::joinWorkers` requests stop, closes the
source and queue, then joins capture then inference. `CameraManager::stop()`
stops the pipeline only; the writer stays up for history queries.

## 3. Inference

Object mode (`YoloDetector`) is the only detector that uses
`IInferenceEngine`. Face uses OpenCV DNN Caffe. Motion uses MOG2. Dummy is a
plugin test double.

```mermaid
flowchart TB
  Yolo["YoloDetector"]
  Factory["createInferenceEngine"]
  Port["IInferenceEngine"]
  CPU["OnnxRuntimeEngine\nonnx-cpu FP32"]
  CUDA["OnnxRuntimeCudaEngine\nonnx-cuda FP32"]
  TRT["TensorRTEngine\ntensorrt FP32 / FP16"]
  Unav["UnavailableInferenceEngine"]

  Yolo --> Factory --> Port
  Port --> CPU
  Port --> CUDA
  Port --> TRT
  Factory -.-> Unav
```

Selection is `ModelConfig.backend` / env `VISIONLAB_INFERENCE_BACKEND`. GPU
initialize failure does **not** fall back to CPU. TensorRT is compiled only
when `VISIONLAB_ENABLE_TENSORRT=ON`. INT8 is not implemented.

## 4. Plugins vs inference backends

These are **different** extension points. Backends are not `IVisionPlugin`
instances. Plugins are not unloaded in V1.

```mermaid
flowchart TB
  PM["PluginManager QPluginLoader"]
  Dummy["vision.dummy"]
  Motion["vision.motion"]
  Face["vision.face"]
  YoloP["vision.yolo"]
  ID["IDetector"]
  Eng["createInferenceEngine"]

  PM --> Dummy --> ID
  PM --> Motion --> ID
  PM --> Face --> ID
  PM --> YoloP --> ID
  YoloP -->|"Object mode only"| Eng
```

Scan directory: `VISIONLAB_PLUGIN_DIR` or `<app>/plugins`. `CameraManager`
declares `PluginManager m_plugins` before `m_pipeline` so detectors die before
`QPluginLoader`. See `docs/plugins/detector-plugins.md`.

## 5. Detect → track → rules → render → EventLog

All of this runs **inside** `InferenceWorker::run` on the inference `jthread`.
Rules are not a separate worker.

```mermaid
flowchart TB
  Pop["BoundedQueue pop FramePacket"]
  Det["IDetector::detect"]
  Track["ITracker::update ByteTrackTracker"]
  Rules["RuleEngine::evaluate"]
  Log["EventLog::push"]
  Draw{"tracks empty?"}
  TR["TrackRenderer"]
  DR["DetectionRenderer"]
  Pub["LatestResult publish PresentedFrame"]
  Stats["StatsProbe"]

  Pop --> Det --> Track --> Rules --> Log
  Rules --> Draw
  Draw -->|no| TR
  Draw -->|yes| DR
  TR --> Pub
  DR --> Pub
  Pub --> Stats
```

Default production engine has **no** ROI / line / loiter / count rules until
`CameraManager::applyRuleSpecs` (stopped pipeline) or `injectRules` copies the
stored `RuleSpec` list. `PresentedFrame.events` is this frame only.
`LatestResult` is newest-wins; sparse events also go to `EventLog`.
`EventWriter` persists EventLog increments from `notifyFrame` on the GUI
thread via a second bounded queue (capacity 1024, DropOldest).

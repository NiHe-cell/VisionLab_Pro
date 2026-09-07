# Test inventory (VisionLab Pro)

This is the CPU/GPU split used by `ctest -L cpu` (CI) and `ctest -L gpu`
(optional TensorRT builds). It is not a coverage-percentage report and it
does not record FPS or latency.

High-risk areas listed in Phase 9 already have `add_test` targets. This
task does not add empty tests for line coverage.

Skip policy: some cases call `QSKIP` when a fixture, model, driver, or GPU
is missing. Others fail hard if a required file is absent (called out below).

## Labels

- `cpu` — default `ctest -L cpu`. Includes `onnxruntimecudaengine` (construct /
  failure paths; identity round-trip `QSKIP`s without CUDA).
- `gpu` — only when `VISIONLAB_ENABLE_TENSORRT=ON`: `tensorrtengine`,
  `tensorrtenginecache`, `cudaraii`.

## Inventory

| Area | CTest name | Source | Missing model / no GPU |
|---|---|---|---|
| Smoke | `smoke` | `tests/SmokeTest.cpp` | runs |
| FramePacket | `framepacket` | `tests/FramePacketTest.cpp` | runs |
| Track domain | `track` | `tests/TrackTest.cpp` | runs |
| VisionEvent | `visionevent` | `tests/VisionEventTest.cpp` | runs |
| ModelConfig | `modelconfig` | `tests/ModelConfigTest.cpp` | runs |
| IInferenceEngine | `iinferenceengine` | `tests/IInferenceEngineTest.cpp` | runs (fake engine) |
| ORT CPU | `onnxruntimeengine` | `tests/OnnxRuntimeEngineTest.cpp` | `QSKIP` if identity fixture or `yolov4-tiny.onnx` missing |
| Engine factory | `inferenceenginefactory` | `tests/InferenceEngineFactoryTest.cpp` | `QSKIP` if identity fixture missing |
| ORT CUDA engine | `onnxruntimecudaengine` | `tests/OnnxRuntimeCudaEngineTest.cpp` | `QSKIP` identity / CUDA init failure; other slots run |
| Inference selection | `inferenceselection` | `tests/InferenceSelectionTest.cpp` | runs |
| Plugin metadata | `pluginmetadata` | `tests/PluginMetadataTest.cpp` | runs |
| IDetector | `idetector` | `tests/IDetectorTest.cpp` | runs |
| ITracker | `itracker` | `tests/ITrackerTest.cpp` | runs |
| IRule | `irule` | `tests/IRuleTest.cpp` | runs |
| Rule geometry | `rulegeometry` | `tests/RuleGeometryTest.cpp` | runs |
| ROI intrusion | `roiintrusion` | `tests/RoiIntrusionRuleTest.cpp` | runs |
| Line crossing | `linecrossing` | `tests/LineCrossingRuleTest.cpp` | runs |
| Loitering | `loitering` | `tests/LoiteringRuleTest.cpp` | runs |
| Counting | `counting` | `tests/CountingRuleTest.cpp` | runs |
| RuleEngine | `ruleengine` | `tests/RuleEngineTest.cpp` | runs |
| RuleSpec / makeRule | `rulespec` | `tests/RuleSpecTest.cpp` | runs |
| SQLite repository | `eventrepository` | `tests/EventRepositoryTest.cpp` | `QSKIP` if QSQLITE driver missing |
| IoU matching | `ioumatching` | `tests/IouMatchingTest.cpp` | runs |
| Kalman box | `kalmanboxfilter` | `tests/KalmanBoxFilterTest.cpp` | runs |
| ByteTrack | `bytetracktracker` | `tests/ByteTrackTrackerTest.cpp` | runs |
| Detection renderer | `detectionrenderer` | `tests/DetectionRendererTest.cpp` | runs |
| Track renderer | `trackrenderer` | `tests/TrackRendererTest.cpp` | runs |
| Letterbox | `letterbox` | `tests/LetterboxTest.cpp` | runs |
| DetectionModel | `detectionmodel` | `tests/DetectionModelTest.cpp` | runs |
| TrackModel | `trackmodel` | `tests/TrackModelTest.cpp` | runs |
| EventModel | `eventmodel` | `tests/EventModelTest.cpp` | runs |
| PerformanceModel | `performancemodel` | `tests/PerformanceModelTest.cpp` | runs |
| PluginModel | `pluginmodel` | `tests/PluginModelTest.cpp` | runs |
| RuleModel | `rulemodel` | `tests/RuleModelTest.cpp` | runs |
| EventWriter | `eventwriter` | `tests/EventWriterTest.cpp` | runs (temp db) |
| VisionController | `visioncontroller` | `tests/VisionControllerTest.cpp` | runs |
| YOLO decode | `yolodecoding` | `tests/YoloDecodingTest.cpp` | runs |
| YOLO preprocess | `yolopreprocess` | `tests/YoloPreprocessTest.cpp` | runs |
| YoloDetector | `yolodetector` | `tests/YoloDetectorTest.cpp` | runs (fake engine) |
| SSD decode | `ssddecoding` | `tests/SsdDecodingTest.cpp` | runs |
| MotionDetector | `motiondetector` | `tests/MotionDetectorTest.cpp` | runs |
| DummyDetector | `dummydetector` | `tests/DummyDetectorTest.cpp` | runs |
| Dummy plugin | `dummyplugin` | `tests/DummyPluginTest.cpp` | runs |
| Motion plugin | `motionplugin` | `tests/MotionPluginTest.cpp` | runs |
| Face plugin | `faceplugin` | `tests/FacePluginTest.cpp` | **fails** if `models/` Caffe files missing (no `QSKIP`) |
| YOLO plugin | `yoloplugin` | `tests/YoloPluginTest.cpp` | `QSKIP` one slot if CUDA EP is present |
| PluginManager | `pluginmanager` | `tests/PluginManagerTest.cpp` | runs |
| IVideoSource | `ivideosource` | `tests/IVideoSourceTest.cpp` | runs |
| CameraSource | `camerasource` | `tests/CameraSourceTest.cpp` | runs |
| Mode mapping | `modemapping` | `tests/ModeMappingTest.cpp` | runs |
| BoundedQueue | `boundedqueue` | `tests/BoundedQueueTest.cpp` | runs |
| LatestResult | `latestresult` | `tests/LatestResultTest.cpp` | runs |
| CaptureWorker | `captureworker` | `tests/CaptureWorkerTest.cpp` | runs |
| InferenceWorker | `inferenceworker` | `tests/InferenceWorkerTest.cpp` | runs |
| LatencyWindow | `latencywindow` | `tests/LatencyWindowTest.cpp` | runs |
| StatsProbe | `statsprobe` | `tests/StatsProbeTest.cpp` | runs |
| EventLog | `eventlog` | `tests/EventLogTest.cpp` | runs |
| VisionPipeline | `visionpipeline` | `tests/VisionPipelineTest.cpp` | runs |
| Pipeline overload | `pipelineoverload` | `tests/PipelineOverloadTest.cpp` | runs (DropOldest; not a latency gate) |
| Pipeline lifecycle | `pipelinelifecycle` | `tests/PipelineLifecycleTest.cpp` | runs |
| CameraManager | `cameramanager` | `tests/CameraManagerTest.cpp` | runs |
| LocaleController | `localecontroller` | `tests/LocaleControllerTest.cpp` | runs |
| Inference bench CLI | `onnxcpubench` | `tests/OnnxCpuBenchmarkTest.cpp` | field names only; no ms threshold |
| Pipeline bench CLI | `pipelinebench` | `tests/PipelineBenchmarkTest.cpp` | field names only; 2 s run |
| Tracker bench CLI | `trackerbench` | `tests/TrackerBenchmarkTest.cpp` | field names only |
| TensorRT engine | `tensorrtengine` | `tests/TensorRTEngineTest.cpp` | label `gpu`; `QSKIP` no device / missing ONNX |
| TensorRT cache | `tensorrtenginecache` | `tests/TensorRTEngineCacheTest.cpp` | label `gpu`; `QSKIP` no device / missing ONNX |
| CUDA RAII | `cudaraii` | `tests/CudaRaiiTest.cpp` | label `gpu`; `QSKIP` no device |

## Commands

```
ctest --test-dir build/dev-debug -N -L cpu
ctest --test-dir build/dev-debug -L cpu --output-on-failure
```

With TensorRT enabled, `ctest -N -L gpu` lists the three GPU tests. With
`VISIONLAB_ENABLE_TENSORRT=OFF` that label set is empty.

GitHub Actions runs the same `ctest -L cpu` on `windows-2022` without CUDA
(`.github/workflows/ci.yml`, `docs/build.md`). Missing `yolov4-tiny.onnx`
must `QSKIP`, not fail the job.

Do not copy Performance-page snapshots or bench stdout into this file as
guaranteed throughput.

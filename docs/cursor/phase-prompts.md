# VisionLab Pro — Cursor Phase 0~9 Development Prompts

---

# 全局上下文

Project Name:

VisionLab Pro

Project Goal:

Transform the existing VisionLab repository from a Qt/QML + OpenCV computer vision demo into a production-oriented real-time intelligent video analytics and edge AI inference platform.

Primary technical directions:

1. Modern C++ architecture.
2. Real-time asynchronous video processing pipeline.
3. Multi-backend AI inference engine.
4. CUDA / TensorRT GPU acceleration.
5. Runtime-loadable detector plugin framework.
6. Multi-object tracking.
7. Intelligent video analytics rule engine.
8. Qt/QML UI and runtime performance visualization.
9. Benchmarking, testing and engineering documentation.

General engineering rules:

* Use C++20 unless an external dependency requires otherwise.
* Prefer RAII and deterministic lifetime management.
* Prefer value semantics where practical.
* Use std::unique_ptr for exclusive ownership.
* Use std::shared_ptr only when ownership is genuinely shared.
* Avoid raw owning pointers.
* Never introduce detached std::thread.
* Prefer std::jthread / std::stop_token for long-lived standard C++ workers.
* Never execute AI inference on the Qt GUI thread.
* Avoid global mutable state.
* No hard-coded absolute Windows paths.
* Core/domain modules must not depend on QML.
* Detector implementations must return structured results.
* Detector implementations must not draw directly onto cv::Mat.
* Video capture, inference, tracking, analytics, rendering and storage must remain decoupled.
* Communication between real-time processing stages must use bounded queues.
* Real-time frame queues must support a DropOldest overflow policy.
* Inference backends must be hidden behind IInferenceEngine.
* Detection algorithms must implement IDetector.
* Tracking algorithms must implement ITracker.
* Analytics rules must implement IRule.
* Runtime detector extensions must use the plugin system.
* QML should consume C++ domain data through Qt Model/View abstractions.
* Respect QObject thread affinity.
* Do not modify unrelated modules.
* Do not perform repository-wide rewrites without explicit approval.
* Each logical task must leave the repository buildable.
* Add tests for non-trivial infrastructure and algorithms.
* Before changing an existing public interface, identify all callers.
* Prefer incremental migration over big-bang refactoring.

Every implementation task must follow this process:

1. Inspect existing related code.
2. Explain current implementation and problems.
3. Propose the smallest architectural change.
4. List files that will be added/modified.
5. Identify compatibility, ownership and concurrency risks.
6. Implement only the approved task.
7. Build the project.
8. Run relevant tests.
9. Review the implementation.
10. Report changed files and remaining technical debt.

Do not silently fix unrelated issues.

---

# Phase 0 — Baseline、代码库分析与工程规范

## Goal

Do not implement new VisionLab Pro functionality yet.

The objective of Phase 0 is to establish a trustworthy development baseline and fully understand the upstream repository before architecture-level refactoring begins.

## Tasks

Analyze the complete current repository.

Specifically investigate:

1. Repository directory structure.
2. CMake organization and dependency discovery.
3. Qt/QML entry points.
4. Camera lifecycle.
5. CameraManager responsibilities.
6. Existing worker/thread model.
7. Existing FaceDetector.
8. Existing ObjectDetector.
9. Existing MotionDetector.
10. How detection mode switching currently works.
11. How cv::Mat reaches QML.
12. Where drawing is performed.
13. Ownership relationships between major objects.
14. Possible data races.
15. Detached threads.
16. Resource lifetime problems.
17. Shutdown behavior.
18. Hard-coded paths.
19. Configuration that is embedded in source code.
20. Model loading behavior.
21. Error handling.
22. Logging.
23. Existing performance measurement capability.
24. Testing capability.

## Architecture Analysis

Produce a current dependency diagram similar to:

Application
→ CameraManager
→ Camera
→ Detector
→ OpenCV
→ QML

but derive the real graph from the codebase.

Identify classes with too many responsibilities.

Identify violations of:

* Single Responsibility Principle
* Dependency Inversion
* Separation of concerns
* UI/domain separation

## Baseline

Create a proposal for collecting baseline metrics without substantially changing behavior.

Metrics should include:

* Capture FPS
* Inference FPS
* Render FPS if measurable
* Average inference latency
* Memory usage where practical
* CPU usage where practical
* Current detector backend
* Input resolution
* Model information

Do NOT invent benchmark numbers.

## Engineering Setup

Recommend:

* C++ standard migration approach.
* Directory restructuring strategy.
* Testing framework integration.
* Logging strategy.
* Configuration strategy.
* .gitignore improvements if necessary.
* Development/build presets if appropriate.

## Project Rules

Create or update an AGENTS.md or equivalent project-level engineering instruction file containing the VisionLab Pro architectural rules.

Do not unnecessarily duplicate documentation.

## Required Output

Do NOT modify production architecture yet.

First provide:

### 1. Current Architecture

Describe how the repository currently works.

### 2. Problems

Categorize issues as:

Critical
High
Medium
Low

### 3. Current Dependency Graph

ASCII diagram.

### 4. Target Dependency Graph

ASCII diagram for VisionLab Pro.

### 5. Migration Strategy

Explain how we can migrate incrementally without breaking current functionality.

### 6. Phase 1 Task Breakdown

Break Phase 1 into small tasks.

Each task should ideally modify one architectural concept.

### 7. Risk List

Include:

* C++ ownership
* cv::Mat lifetime
* Qt thread affinity
* API breakage
* model compatibility
* build system risks

Do not begin Phase 1 implementation until I explicitly request a specific task.

---

# Phase 1 — Core Architecture Refactoring

## Goal

Refactor the existing tightly coupled VisionLab implementation into a clean domain-driven core without introducing new AI functionality.

Visible application behavior should remain approximately unchanged.

The key goal is:

Separate domain data, video acquisition and detector logic from CameraManager and rendering.

## Target Components

Introduce or evolve toward:

core/
FramePacket
Detection
VisionTypes
PipelineStats

video/
IVideoSource
CameraSource
VideoFileSource if appropriate for this phase

detector/
IDetector

application/pipeline/
orchestration layer

Do not implement GPU inference, tracking, analytics or dynamic plugins yet.

## Domain Types

Design FramePacket.

It should contain at least:

* frameId
* capture timestamp
* sourceId
* cv::Mat or a carefully designed frame representation

Discuss cv::Mat ownership semantics before implementation.

Design Detection.

It should contain at least:

* classId
* label
* confidence
* bounding box

Avoid Qt UI types in pure domain structures unless there is a strong reason.

## Detector Refactoring

Create an IDetector abstraction.

Existing:

* ObjectDetector
* FaceDetector
* MotionDetector

should progressively implement the abstraction.

Current detector logic that draws rectangles/text directly onto frames must be separated.

Target flow:

Frame
→ Detector
→ vector<Detection>

Rendering must become a separate concern.

If MotionDetector requires different output semantics, propose a clean generalized design rather than forcing an incorrect abstraction.

## Video Source

Create IVideoSource.

Responsibilities:

* open
* read
* close
* state query
* source identity

Implement CameraSource using the current local camera behavior.

Do not introduce excessive abstractions.

## CameraManager

Reduce CameraManager responsibilities.

CameraManager should no longer directly own every concrete detector.

Remove string-based algorithm switching when practical.

If it cannot all be removed safely in one change, provide an incremental migration path.

## CMake

Remove absolute local paths such as hard-coded OpenCV directories.

Use proper CMake dependency discovery.

Keep Windows development usability.

## Required Process

Before coding:

1. Inspect every caller affected.
2. Produce proposed interfaces.
3. Explain ownership.
4. Explain migration sequence.
5. List modified files.

Then implement Phase 1 as small independent tasks.

Do NOT implement all tasks in one change.

## Suggested Task Order

Task 1:
Introduce core domain types.

Task 2:
Introduce IDetector.

Task 3:
Refactor ObjectDetector to return structured Detection results.

Task 4:
Refactor FaceDetector.

Task 5:
Refactor MotionDetector.

Task 6:
Introduce IVideoSource.

Task 7:
Implement CameraSource.

Task 8:
Reduce CameraManager responsibilities.

Task 9:
Separate rendering from detection.

Task 10:
Clean CMake configuration.

## Definition of Done

Phase 1 is complete only when:

* Existing major detection functionality still works.
* Detector implementations no longer render bounding boxes themselves.
* Detector output is structured domain data.
* Camera capture is abstracted.
* CameraManager is significantly less coupled.
* Concrete detectors are not selected through a growing string if/else chain.
* No new detached threads are introduced.
* Core modules do not depend on QML.
* Build succeeds.
* Existing functionality is smoke tested.
* Relevant unit tests exist.

After implementation, produce a Phase 1 architecture review.

---

# Phase 2 — Real-Time Multithreaded Pipeline

## Goal

Build a low-latency asynchronous real-time video processing pipeline.

The application must remain responsive even if inference is slower than camera capture.

Do not add TensorRT, tracking or analytics yet.

## Architecture

Target flow:

CaptureWorker
→ Bounded Frame Queue
→ InferenceWorker
→ Result Queue / Latest Result
→ UI

Design the system so Tracking and Analytics stages can be inserted later without rewriting the whole pipeline.

## BoundedQueue

Implement a reusable:

template<typename T>
class BoundedQueue

Requirements:

* C++20.
* Thread-safe.
* Fixed capacity.
* condition_variable based where appropriate.
* Move support.
* close() or equivalent shutdown behavior.
* Safe destruction.
* No Qt dependency.
* No busy waiting.

Overflow policies:

* BlockProducer
* DropNewest
* DropOldest

Real-time frame processing should use DropOldest by default.

Before implementation explain:

* push semantics
* pop semantics
* close semantics
* behavior after close
* behavior when full
* behavior during shutdown
* exception safety

Add thorough unit tests.

## Capture Worker

Separate video acquisition into a dedicated worker.

Responsibilities:

* Read from IVideoSource.
* Generate monotonically increasing frame IDs.
* Timestamp frames immediately after capture.
* Push into bounded queue.
* Track capture metrics.
* Stop cleanly.

Prefer std::jthread and std::stop_token unless Qt ownership makes another approach clearly superior.

Explain the chosen threading model.

## Inference Worker

Responsibilities:

* Consume newest available frames.
* Execute detector.
* Publish structured results.
* Record inference timing.
* Stop cleanly.

UI must never block waiting for inference.

## Backpressure

Explicitly test:

Camera producer = 30/60 FPS
Inference consumer = intentionally slower

Verify:

* Queue remains bounded.
* Memory remains stable.
* Old frames are dropped.
* End-to-end latency remains controlled.
* Application remains responsive.

## Metrics

Create PipelineStats with at least:

* capture FPS
* inference FPS
* render FPS if available
* captured frames
* processed frames
* dropped frames
* queue depth
* average inference latency
* P50 latency
* P95 latency
* end-to-end latency

Avoid expensive synchronization on every UI update if possible.

## Lifecycle

Pay special attention to:

start()
stop()
restart()
source failure
detector failure
application shutdown

There must be no detached background workers after shutdown.

## Tests

Add tests for:

* BoundedQueue push/pop
* DropOldest
* DropNewest
* blocking behavior
* close while producer waits
* close while consumer waits
* multiple producers/consumers where supported
* repeated start/stop
* pipeline overload

## Definition of Done

Phase 2 is complete only when:

* Capture and inference are decoupled.
* GUI never runs inference.
* Queue is bounded.
* DropOldest works.
* No unbounded latency growth.
* start/stop is deterministic.
* Shutdown has no hanging threads.
* droppedFrames is measurable.
* end-to-end latency is measurable.
* tests pass.

After implementation, perform a dedicated concurrency review focusing on:

data races
deadlocks
lifetime
cv::Mat ownership
shutdown
thread affinity

---

# Phase 3 — Inference Engine Abstraction + ONNX Runtime CPU

## Goal

Decouple detector algorithms from the concrete inference runtime.

Introduce a reusable inference engine architecture and implement ONNX Runtime CPU as the first production backend.

Do not implement CUDA/TensorRT in this phase.

## Architecture

Target relationship:

YoloDetector
→ IInferenceEngine
→ OnnxRuntimeEngine

The detector owns:

* computer vision preprocessing semantics if model-specific
* output decoding
* NMS if model-specific

The inference engine owns:

* runtime session
* model loading
* tensor execution
* backend-specific runtime resources

Carefully determine where preprocessing should live.

Do not create a generic abstraction that makes model-specific operations unnatural.

## IInferenceEngine

Design an interface supporting:

* model load/init
* inference
* backend identification
* device information
* input/output metadata
* explicit initialization failure reporting

Avoid exposing ONNX Runtime classes through public engine-independent interfaces.

## ModelConfig

Design configuration for:

* model path
* input width
* input height
* confidence threshold
* NMS threshold
* backend
* device id
* optional class names path
* precision where appropriate later

Use filesystem paths appropriately.

## ONNX Runtime CPU

Implement:

OnnxRuntimeEngine

Requirements:

* RAII for session/environment/resources.
* No global mutable runtime state.
* Clear error propagation.
* Cache input/output names or metadata when appropriate.
* Avoid repeated allocation where practical.
* Support model warmup.
* Record inference latency.

## YOLO Migration

Replace the old OpenCV DNN coupling in the object detector with the new inference abstraction.

Use an ONNX-format YOLO model appropriate for the project.

Do not mix model download logic into the inference runtime.

Document expected model format.

## Preprocess

Implement and test:

* resize / letterbox as required
* color conversion
* normalization
* HWC → CHW
* tensor layout

Store enough metadata to accurately map detection coordinates back to original image dimensions.

## Postprocess

Implement and test:

* output tensor parsing
* confidence filtering
* class selection
* coordinate restoration
* NMS
* Detection generation

## Tests

Where practical test preprocessing and postprocessing independently from the runtime.

Provide deterministic synthetic inputs for tests.

## Performance

Add a basic CPU inference benchmark.

Measure:

* warmup count
* iteration count
* mean latency
* P50
* P95
* throughput

Do not invent performance numbers.

## Definition of Done

Phase 3 is complete only when:

* YoloDetector no longer directly owns cv::dnn::Net.
* IInferenceEngine exists.
* ONNX Runtime CPU backend works.
* Model loading errors are handled.
* Preprocess/postprocess are testable.
* Detection coordinates are correct.
* Pipeline still works.
* CPU benchmark works.

After finishing, explain exactly what code will remain unchanged when CUDA/TensorRT is introduced.

---

# Phase 4 — CUDA + TensorRT GPU Acceleration

## Goal

Add NVIDIA GPU inference support without modifying higher-level detector, tracking or UI architecture.

Backends should be selectable at runtime/configuration level.

Target:

IInferenceEngine
├── OnnxRuntimeCpuEngine
├── OnnxRuntimeCudaEngine
└── TensorRTEngine

Optional OpenVINO support may be planned but should not distract from NVIDIA implementation.

## Part A — ONNX Runtime CUDA

Implement a CUDA-backed ONNX Runtime engine.

Requirements:

* Select device by device ID.
* Validate CUDA provider availability.
* Provide useful initialization errors.
* Gracefully handle machines without supported GPU runtime.
* Preserve CPU fallback strategy at application level.
* Keep YoloDetector unchanged.

Benchmark against ORT CPU.

## Part B — TensorRT

Before coding TensorRT implementation, produce a design document covering:

1. ONNX parsing.
2. TensorRT builder.
3. Network definition.
4. optimization profiles if needed.
5. engine serialization.
6. engine deserialization.
7. execution context.
8. input/output bindings or tensor API according to installed TensorRT version.
9. CUDA stream lifecycle.
10. host/device buffer lifecycle.
11. dynamic shape handling.
12. FP32.
13. FP16.
14. engine caching.
15. warmup.

Do not write TensorRT code until the design is explained.

## TensorRTEngine

Implement with strict RAII.

No leaking:

* CUDA streams
* device memory
* TensorRT runtime
* engine
* context

Avoid raw cudaMalloc/cudaFree scattered across the codebase.

Consider dedicated RAII buffer wrappers.

## Engine Cache

Implement cache semantics based on enough information to avoid incompatible reuse.

Consider:

* model identity
* GPU
* precision
* TensorRT version
* shape/profile

Do not over-engineer but avoid blindly loading stale engines.

## Precision

Support:

FP32

and:

FP16

only when supported.

INT8 should be left for a future phase unless all other Phase 4 goals are complete.

## Runtime Selection

Allow configuration such as:

backend = onnx-cpu
backend = onnx-cuda
backend = tensorrt

precision = fp32 / fp16

Device/backend selection should not require recompiling detector code.

## Failure Handling

Examples:

* TensorRT unavailable
* GPU unavailable
* CUDA out of memory
* incompatible serialized engine
* unsupported model
* invalid device ID

Return meaningful errors.

Do not silently hide severe failures.

## Benchmark

Build a benchmark comparing:

* ONNX Runtime CPU
* ONNX Runtime CUDA
* TensorRT FP32
* TensorRT FP16

Record:

* warmup
* P50
* P95
* P99
* throughput
* end-to-end pipeline FPS where relevant
* GPU memory where practical

Do not fabricate results.

## Definition of Done

* Detector code is backend-independent.
* ORT CPU works.
* ORT CUDA works.
* TensorRT FP32 works.
* TensorRT FP16 works where supported.
* Engine cache works.
* GPU resources use RAII.
* GPU unavailable behavior is defined.
* Benchmark exists.
* No UI-thread GPU inference.
* Pipeline remains stable.

After implementation perform a GPU-specific review focusing on:

resource ownership
device synchronization
unnecessary memcpy
allocation frequency
stream synchronization
engine cache correctness

---

# Phase 5 — Detector Plugin Framework

## Goal

Allow detector functionality to be added without modifying VisionLab Pro core application source code.

The goal is a runtime-discoverable Qt/C++ detector plugin architecture.

Do not confuse detector plugins with inference engines.

Inference backend and detector algorithm are separate extension points.

## Architecture

Target:

PluginManager
→ IVisionPlugin
→ creates IDetector

Example:

plugins/
vision_yolo
vision_face
vision_motion
vision_dummy

The executable should discover compatible plugins from a configured plugin directory.

## Interface Design

Design:

PluginMetadata

including:

* plugin id
* name
* version
* description
* capabilities

Design:

IVisionPlugin

Responsibilities:

* expose metadata
* create detector instance
* report compatibility if needed

Use Qt plugin mechanisms appropriately.

Avoid exposing unnecessary implementation details through plugin ABI.

## PluginManager

Responsibilities:

* scan directory
* load plugins
* validate interface
* collect metadata
* report errors
* create detector instances
* manage plugin loader lifetime safely

Think carefully about DLL/shared-library unloading.

Do not unload a plugin while objects created by it still exist.

If safe runtime unloading is complex, explicitly keep plugins loaded until application shutdown.

That is acceptable for V1.

## Migration

Move existing detectors progressively into plugins.

Recommended order:

1. DummyDetectorPlugin
2. MotionDetectorPlugin
3. FaceDetectorPlugin
4. YoloDetectorPlugin

Start with DummyDetectorPlugin to prove architecture.

Dummy detector should return deterministic fake Detection data.

## Plugin Discovery

Expected behavior:

Add plugin library
→ restart application
→ plugin appears

Remove plugin library
→ restart application
→ plugin disappears

Core executable must not require source changes.

## Errors

Handle:

* invalid plugin
* wrong interface version
* dependency load failure
* duplicate plugin id
* detector creation failure

Expose useful diagnostic logs.

## Tests

Test:

* empty directory
* valid dummy plugin
* invalid library
* duplicate id
* failed detector creation
* lifetime behavior

## Definition of Done

* Detector extension no longer requires editing central if/else logic.
* Dummy plugin proves runtime discovery.
* Existing key detector can run through plugin system.
* PluginManager owns/retains loaders correctly.
* Plugin failures do not crash application.
* Main application does not depend on concrete detector headers.
* Tests pass.

After implementation explain plugin ABI/lifetime limitations clearly in documentation.

---

# Phase 6 — Multi-Object Tracking

## Goal

Transform frame-level Detection results into persistent tracked objects with stable track IDs and trajectories.

Use an ITracker abstraction.

Initial implementation should integrate ByteTrack or another justified MOT algorithm.

Do not place tracking logic inside detector plugins.

## Architecture

DetectionResult
→ ITracker
→ vector<Track>

Target pipeline:

Capture
→ Detection
→ Tracking
→ Analytics
→ UI

## Track Domain Model

Design Track with:

* trackId
* classId
* label
* confidence
* bounding box
* state
* firstSeen
* lastSeen
* age
* trajectory/history

Avoid keeping an unlimited trajectory vector.

Use bounded history or another memory-safe strategy.

## ITracker

Design:

update(detections, timestamp/frame information)
reset()

Possibly expose configuration.

Keep tracking algorithm implementation hidden.

## ByteTrack Integration

Before implementation:

Explain ByteTrack algorithm at a practical engineering level:

* high-confidence detections
* low-confidence detections
* association
* track lifecycle
* lost tracks
* removed tracks
* IoU matching or related mechanism

Identify external code/license implications if integrating third-party implementation.

Prefer a clean adaptation rather than copying a large uncontrolled implementation.

## Pipeline Integration

Tracking should not unnecessarily block capture.

Decide whether tracking runs:

* inside inference worker after detector
  or
* as its own pipeline stage

Base decision on complexity and current pipeline design.

Document the tradeoff.

## UI Output

Expose:

Person #17
Car #8

with bounding box and optional trajectory.

Do not let QML directly manipulate tracker internals.

## Metrics

Track:

* active tracks
* created tracks
* lost tracks
* removed tracks
* tracking latency
* tracking FPS if useful

## Tests

Use synthetic sequences to test:

* stable ID across nearby frames
* temporary missed detection
* new track creation
* track expiration
* reset behavior
* trajectory bound
* no detections

## Definition of Done

* Persistent track IDs exist.
* Tracker implements ITracker.
* Detector remains unaware of tracker.
* Track lifecycle is defined.
* Memory does not grow indefinitely.
* UI can display IDs.
* Pipeline remains responsive.
* Tracking tests pass.

After implementation review ID stability, memory behavior and tracker reset semantics.

---

# Phase 7 — Intelligent Event / Rule Engine

## Goal

Build a configurable intelligent video analytics layer operating on tracked objects.

Initial V1 rules:

1. ROI intrusion.
2. Line crossing.
3. Loitering.
4. Counting.

Do not embed business rules inside the tracker.

## Architecture

Tracks
→ RuleEngine
→ IRule[]
→ VisionEvent[]

## Domain Types

Design:

VisionEvent

containing where appropriate:

* event id
* event type
* source id
* track id
* object class
* timestamp
* confidence
* bounding box
* rule id
* message
* snapshot reference if added later

Design rule configuration structures.

## IRule

Provide clean interface such as:

evaluate(context, tracks)
→ vector<VisionEvent>

Also consider:

reset()

Per-source state must be handled correctly.

## RuleEngine

Responsibilities:

* register rules
* enable/disable rules
* evaluate active rules
* collect events
* prevent event spam
* isolate state per camera/source where necessary

## ROI Intrusion

Support polygon/rectangle ROI.

Define clearly when an object counts as inside.

Possible strategies:

* bounding-box center
* foot point / bottom center
* overlap ratio

Choose and document one default.

Generate event on transition:

outside → inside

Do not generate the same event every frame.

## Line Crossing

Detect actual directional crossing.

Support:

A → B
B → A

Avoid simply checking rectangle-line intersection every frame.

Use previous and current tracked-object positions.

## Loitering

Maintain entry timestamp per track/rule.

Generate event when:

inside ROI continuously for configured threshold.

Handle:

track lost
track leaves
track re-enters
rule reset

## Counting

Build directional IN / OUT counting based on line crossing.

Avoid double counting the same track.

Support occupancy calculation when meaningful.

## Cooldown / Deduplication

Design an event deduplication mechanism.

Prevent:

frame1 alert
frame2 alert
frame3 alert
...

Rules should emit transition/threshold events rather than frame-state spam.

## Tests

Create strong deterministic tests for:

ROI:

* outside
* enter
* stay
* exit
* re-enter

Line:

* A→B
* B→A
* parallel movement
* touch without crossing

Loiter:

* below threshold
* threshold reached
* exit before threshold
* re-entry

Counting:

* correct IN/OUT
* no duplicate count

## Definition of Done

* IRule abstraction exists.
* RuleEngine is detector/tracker independent.
* ROI intrusion works.
* Line crossing works directionally.
* Loitering works.
* Counting works.
* Event duplication is controlled.
* Rules are configurable.
* Tests cover geometry/state edge cases.

After implementation produce an explanation of state machines used by each rule.

---

# Phase 8 — Qt/QML UI + Data Visualization + Event Storage

## Goal

Build a professional monitoring interface around the existing backend architecture.

The UI must visualize domain data without owning inference/tracking/business logic.

Primary screens:

1. Monitor.
2. Events.
3. Performance.
4. Settings.

## C++ / QML Boundary

Create appropriate Qt models.

Candidates:

* DetectionModel
* TrackModel
* EventModel
* PluginModel
* PerformanceModel

Prefer QAbstractListModel/QAbstractItemModel where appropriate.

Avoid repeatedly copying large frame or event data into QML unnecessarily.

## Monitor Page

Display:

* video
* bounding boxes
* labels
* confidence
* track IDs
* trajectories if enabled
* ROI
* line rules
* rule state indicators where useful

Provide interaction for drawing/configuring:

* ROI polygon/rectangle
* line crossing line

Keep geometry transformations well-defined between:

original frame coordinates
displayed image coordinates

Pay attention to letterboxing / aspect ratio.

## Performance Page

Visualize:

* capture FPS
* inference FPS
* render FPS
* P50 latency
* P95 latency
* E2E latency
* queue depth
* dropped frames
* active tracks
* event rate
* selected backend
* precision
* device

Avoid updating QML at extremely high frequencies.

Use an appropriate telemetry update interval.

## Settings

Expose where implemented:

* detector plugin
* model
* backend
* device
* confidence
* NMS
* precision
* tracking enable
* rule enable
* rule parameters

Settings changes that require pipeline restart should be clearly modeled.

Do not introduce race conditions by mutating live engine configuration from QML threads.

## Events Page

Display:

* timestamp
* source
* event type
* track ID
* class
* confidence
* event message
* optional snapshot

Support filtering if practical.

## SQLite Event Storage

Introduce:

IEventRepository or equivalent abstraction

and:

SQLiteEventRepository

Persist VisionEvent.

Responsibilities:

* schema initialization
* insert
* basic query
* filtering
* retention strategy proposal

Do not execute heavy database operations on GUI thread.

## UI Architecture

Target:

C++ Domain
→ Application/View Models
→ QAbstractListModel
→ QML

QML must not access detector/tracker/rule engine implementation objects directly.

## Tests

Test model roles and updates where practical.

Test SQLite:

* schema
* insert
* query
* filtering
* malformed data/error handling

## Definition of Done

* UI remains responsive.
* Track overlays work.
* Event page works.
* Performance telemetry works.
* Settings can select implemented options safely.
* ROI/line configuration reaches RuleEngine correctly.
* QML does not own core business logic.
* SQLite stores/query events.
* No high-frequency unnecessary UI allocations.

After implementation perform a QML/C++ boundary review focusing on threading and data-copy overhead.

---

# Phase 9 — Benchmark、Testing、CI、Documentation 与 Release

## Goal

Turn VisionLab Pro from a working development project into a portfolio-quality engineering project suitable for GitHub and technical interviews.

Do not add major new features in Phase 9.

Focus on proving correctness, stability, performance and engineering quality.

## Benchmark Suite

Create dedicated benchmark tools where appropriate:

benchmarks/
inference_benchmark
pipeline_benchmark
tracker_benchmark

### Inference Benchmark

Arguments should support:

* model
* backend
* device
* precision
* iterations
* warmup

Output:

* mean latency
* P50
* P95
* P99
* throughput

Backend comparison:

* ONNX Runtime CPU
* ONNX Runtime CUDA
* TensorRT FP32
* TensorRT FP16

Only include actually implemented/supported backends.

Never invent benchmark numbers.

### Pipeline Benchmark

Simulate or configure overload.

Examples:

producer 60 FPS
consumer 15 FPS

Measure:

* queue depth
* drop rate
* E2E latency
* memory stability
* shutdown behavior

Demonstrate why DropOldest improves real-time behavior.

### Long-Running Stability Test

Run pipeline for an extended period.

Observe:

* memory growth
* crashes
* thread count
* queue stability
* GPU memory
* event accumulation

Provide instructions for reproducing this test.

## Testing

Audit existing tests.

Target coverage areas:

core/

* BoundedQueue

inference/

* preprocess
* postprocess
* config

pipeline/

* start/stop
* overload
* shutdown

plugin/

* discovery
* invalid plugin
* lifetime

tracking/

* lifecycle
* ID persistence

analytics/

* ROI
* crossing
* loitering
* counting

storage/

* SQLite

Do not chase meaningless line coverage.

Prioritize high-risk logic.

## Static / Runtime Quality

Review for:

* compiler warnings
* sanitizer compatibility where possible
* resource leaks
* data races
* undefined behavior
* unnecessary copies
* exception safety

Provide recommendations for:

AddressSanitizer
UndefinedBehaviorSanitizer
ThreadSanitizer where environment permits

Do not force unsupported combinations into the default build.

## CMake Engineering

Review:

* targets
* dependencies
* compile options
* Debug/Release
* optional CUDA/TensorRT features
* optional tests
* optional benchmarks
* installation/runtime plugin paths

Prefer target-based CMake.

Avoid globally leaking include/link directories.

## CI

Create a practical GitHub Actions strategy.

CPU build/test should run without requiring NVIDIA GPU.

GPU/TensorRT support should be optional and documented.

CI should at least validate:

* configure
* build
* unit tests

Avoid designing CI that cannot realistically execute.

## Documentation

Rewrite README as an engineering portfolio document.

Required sections:

# VisionLab Pro

## Overview

## Features

## Demo

## Architecture

## Real-Time Pipeline

## Inference Engine Architecture

## GPU Acceleration

## Plugin Framework

## Multi-Object Tracking

## Analytics Rule Engine

## UI

## Performance

## Project Structure

## Build

## Configuration

## Testing

## Benchmark

## Roadmap

## Upstream Project / Attribution

Clearly state that VisionLab Pro is an architecture-level secondary development based on the original open-source VisionLab repository.

Clearly distinguish upstream functionality from newly implemented functionality.

## Architecture Diagrams

Produce Mermaid or ASCII diagrams for:

1. Overall architecture.
2. Threading model.
3. Inference backend abstraction.
4. Plugin architecture.
5. Detection → Tracking → Analytics flow.

## Performance Documentation

Create a benchmark table template populated only with real measured results.

Example structure:

Backend | Precision | FPS | P50 | P95 | GPU Memory

Never fabricate values.

Document:

hardware
OS
model
resolution
runtime versions

## Interview Documentation

Create:

docs/interview-notes.md

Summarize the major engineering stories:

1. Why bounded queues are required.
2. Why DropOldest was chosen.
3. How shutdown/lifetime is handled.
4. Why inference backend is abstracted.
5. TensorRT optimization strategy.
6. Plugin architecture.
7. Difference between detection and tracking.
8. Rule engine design.
9. Performance optimization methodology.
10. Major bugs/problems encountered and how they were solved.

Do not write fake experiences.

Base everything on actual repository implementation.

## Resume Evidence

Produce a factual summary of measurable project achievements.

Use placeholders where benchmark measurements do not yet exist.

Example:

* Reduced P95 inference latency from [BASELINE] ms to [FINAL] ms using [OPTIMIZATION].
* Increased inference throughput from [BASELINE] FPS to [FINAL] FPS.
* Maintained E2E latency below [VALUE] ms under [LOAD].
* Supported [N] detector plugins.
* Implemented [N] analytics event rules.

Do not invent values.

## Final Architecture Review

Review the entire codebase for:

* module boundaries
* dependency direction
* ownership
* thread safety
* GUI thread isolation
* resource lifetime
* inference abstraction leaks
* plugin lifetime
* duplicated responsibilities
* test gaps
* performance bottlenecks

Classify remaining issues:

Blocker
High
Medium
Low
Future Improvement

## Definition of Done

VisionLab Pro V1.0 is complete only when:

* Clean build works.
* CPU build works without GPU.
* Tests pass.
* Real-time Pipeline is stable.
* ONNX CPU works.
* CUDA backend works where environment supports it.
* TensorRT works where environment supports it.
* Detector plugin architecture works.
* Multi-object tracking works.
* Analytics rules work.
* QML dashboard works.
* Event persistence works.
* Benchmarks are reproducible.
* Documentation describes actual behavior.
* Upstream attribution is clear.
* No fake benchmark or resume claims exist.

At the end, provide a V1.0 release checklist but do not implement new major features.

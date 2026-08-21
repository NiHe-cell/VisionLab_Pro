# TensorRT 10 Engine Design (VisionLab Pro Phase 4)

This document is the implementation contract for `TensorRTEngine`.
T05–T07 must follow it. Changing a decision means editing this file first.

V1 does **not** support TensorRT 8.x bindings, INT8, dynamic shapes,
letterbox, or OpenVINO.

## Locked versions and APIs

- TensorRT **10.x** only.
- Build network with `nvinfer` + `nvonnxparser`.
- Execute with **named I/O tensors** and `IExecutionContext::enqueueV3`.
- CMake option `VISIONLAB_ENABLE_TENSORRT` defaults **OFF**. If TensorRT 10
  headers/libs are not found, configure must fail when the option is ON;
  do not silently compile a stub that pretends to be ready.
- Public headers of `IInferenceEngine` / `TensorRTEngine` must not include
  NvInfer or CUDA types.

---

## 1. ONNX parsing

- Parser: `nvonnxparser::IParser` created from the TensorRT network.
- Input file: `ModelConfig.modelPath` (production Object mode:
  `yolov4-tiny.onnx`). Tests may use `identity_f32_1x3x2x2.onnx`.
- Parse the whole model. If `parse()` returns false, `initialize` fails.
  `lastError` concatenates parser errors (`getError(i)->desc()`).
- Do not rewrite the ONNX graph in VisionLab. If a layer is unsupported,
  fail initialize; do not fall back to ORT.

## 2. TensorRT builder

- Create `IBuilder` from `ILogger` owned by the engine Impl (RAII).
- Create `IBuilderConfig`.
- Memory: use builder default workspace; do not expose a workspace knob
  in `ModelConfig` for V1.
- Timing cache: not used in V1.
- Strongly-typed networks: follow TensorRT 10 defaults; do not enable
  extra flags except FP16 when section 13 applies.

## 3. Network definition

- Explicit batch, batch size **1**.
- Single input, NCHW `float32`, static shape `1 × 3 × H × W` where
  `H = ModelConfig.inputHeight` and `W = ModelConfig.inputWidth`
  (production: **320 × 320**, matching `models/README.md`).
- After parse, inspect the input tensor:
  - rank 4, dtype `FLOAT`, no `-1` dimensions;
  - if the ONNX input is named, keep that name for enqueueV3 bindings.
- Outputs: one or more `float32` tensors. Copy all outputs back to host
  `TensorView`s in parser order so `YoloDetector` can keep using
  `yolo::decodeDetections` unchanged.

## 4. Optimization profiles

- **Not used in V1.** Static shape only.
- If any input/output dimension is `-1`, `initialize` fails with
  `lastError` containing `dynamic`. Do not add an `IOptimizationProfile`.

## 5. Engine serialization

- After a successful build, `builder->buildSerializedNetwork(network, config)`
  (TensorRT 10) yields an `IHostMemory` blob.
- Persist that blob to the cache file described in section 14 when the
  build was not a cache hit.
- Serialization failure → `initialize` fails; do not leave a truncated file
  behind (write to a `.tmp` name then rename).

## 6. Engine deserialization

- Cache hit: `IRuntime::deserializeCudaEngine(blob)`.
- After deserialize, **re-validate** input rank/dtype/shape against
  `1 × 3 × H × W` and `float32`. Mismatch → treat as incompatible cache
  (section 14), do not `isReady()`.
- Runtime and engine objects stay in the Impl until `reset()` / destructor.

## 7. Execution context

- One `IExecutionContext` per `TensorRTEngine` instance.
- Created after the engine is available (build or deserialize).
- Destroyed in Impl `reset()` before the engine.
- Not shared across threads. Same contract as `IDetector`: one inference
  worker thread calls `infer`.

## 8. TensorRT 10 I/O tensor API

- Use `setInputTensorAddress` / `setTensorAddress` (or the 10.x equivalent
  name on the installed headers) with **tensor names** from the engine.
- Do **not** implement the TensorRT 8 `enqueueV2` binding-index API.
- Host tensors from `IInferenceEngine` remain `TensorView` (`vector<float>`
  on CPU). Device pointers live only inside `CudaDeviceBuffer`.

## 9. CUDA stream lifecycle

- Type: `CudaStream` (T05). Move-only. Constructor `cudaStreamCreate`,
  destructor `cudaStreamDestroy`.
- One stream per engine instance, created in `initialize`, destroyed in
  `reset()`.
- `enqueueV3(stream)` then `cudaStreamSynchronize(stream)` before copying
  outputs to `InferResult` and stopping the latency timer.
- Do not use the default stream (0) for V1 execution.

## 10. Host / device buffer lifecycle

- Type: `CudaDeviceBuffer` (T05). Move-only.
  - construct with byte size → `cudaMalloc`
  - destructor → `cudaFree`
  - `copyFromHost(const void*, size, stream)` / `copyToHost(void*, size, stream)`
- One device buffer per engine input tensor and per output tensor, sized
  from the static shape × `sizeof(float)`.
- Reuse buffers across `infer` / `warmup` calls. Do not `cudaMalloc` on
  the hot path.
- **Forbidden:** raw `cudaMalloc` / `cudaFree` / `cudaStreamCreate` in
  `TensorRTEngine.cpp`. All of that goes through T05 wrappers.
- Host input: copy `TensorView.data` into the input device buffer (ORT-style
  host float layout, NCHW). No IOBinding abstraction beyond these copies.

## 11. Dynamic shape handling

- Unsupported. Fail in `initialize` as in section 4.
- Do not call `setInputShape` for V1.

## 12. FP32

- Default `ModelConfig.precision == Fp32`.
- Builder: no FP16 flag.
- Cache key uses `fp32` (section 14).
- Identity fixture and YOLOv4-tiny ONNX both run in FP32.

## 13. FP16

- Only when **all** of the following hold:
  1. `config.precision == InferencePrecision::Fp16`
  2. `builder->platformHasFastFp16() == true`
  3. `VISIONLAB_ENABLE_TENSORRT` is ON and TensorRT 10 is linked
- Then set the FP16 builder flag (TensorRT 10 `BuilderFlag::kFP16`).
- If the caller asked for FP16 but `platformHasFastFp16()` is false:
  `initialize` returns false, `lastError` contains `Fp16` and
  `not supported`. **Do not** build or load an FP32 engine instead.
- ORT CUDA / ORT CPU remain FP32-only (unchanged). INT8 is out of scope.

## 14. Engine cache

Directory: `<parent of onnx>/.trt-cache/`  
Create it on demand. Gitignore `.trt-cache/` and `*.engine` in T05.

File name (no extra hash file):

```
<stem>-s<size>-t<mtime_utc_sec>-g<gpu_slug>-v<trt_version>-p<fp32|fp16>-<1x3xHxW>.engine
```

Field rules:

| Field | Source |
| --- | --- |
| `stem` | ONNX filename stem (`yolov4-tiny`) |
| `size` | `std::filesystem::file_size` of the ONNX |
| `mtime_utc_sec` | last write time as UTC unix seconds |
| `gpu_slug` | sanitized `cudaGetDeviceProperties().name` (alnum + `-` only) |
| `trt_version` | `NV_TENSORRT_MAJOR.MINOR.PATCH` from headers |
| `p` | `fp32` or `fp16` matching `ModelConfig.precision` |
| shape | `1x3xHxW` from config |

Load path:

1. Compute the expected filename.
2. If the file exists, try deserialize + shape/dtype check.
3. On any failure (truncated file, deserialize exception, shape mismatch,
   TRT error): **delete that file**, log why, then rebuild from ONNX.
4. Never execute a blob that failed validation.

A precision or GPU-name change produces a different filename, so the old
file is not reused. Do not hash file contents in V1; a copied ONNX with a
new mtime is allowed to miss and rebuild.

Two processes writing the same path are undefined in V1. Tests must not
do that.

## 15. Warmup

- Same contract as `OnnxRuntimeEngine::warmup`:
  if not ready or `iterations <= 0`, return;
  otherwise `infer` a zero-filled `TensorView` with the engine input shape,
  `iterations` times.
- Counted in `InferResult.latencyMs` only for the timed `infer` used by
  benchmarks, not for warmup itself (callers time the loop they care about).

---

## Failure modes (`lastError`, never silent)

| Situation | `isReady` | Notes |
| --- | --- | --- |
| `VISIONLAB_ENABLE_TENSORRT` OFF | false | Factory returns `UnavailableInferenceEngine`; error contains `not enabled` / `build time` |
| TensorRT / CUDA toolkit missing at configure | n/a | CMake fails when the option is ON |
| No GPU / `cudaGetDeviceCount()==0` | false | error contains `CUDA` or `device` |
| Invalid `deviceId` | false | error contains `device` |
| CUDA OOM | false | error from CUDA/TRT; not ready |
| ONNX parse failure | false | parser messages |
| Unsupported / dynamic model | false | contains `dynamic` or parser error |
| Dirty / incompatible cache | rebuild or false | never run the dirty blob |
| FP16 requested but unsupported | false | contains `Fp16`; no FP32 substitute |
| `backend != TensorRT` | false | engine-specific reject, like ORT engines |

Catch TensorRT/CUDA exceptions inside `initialize` / `infer`. After catch,
`reset()` GPU objects, `isReady()==false`. Never continue as ready.

Do **not** fall back to `OnnxRuntimeEngine` or CUDA EP from this class.

---

## Threading and lifetime

- `initialize` may run on the GUI / assembly thread at startup (same as
  ORT session load). Building a YOLO engine can take seconds; that is
  accepted for V1. Do not move initialize onto a detached thread.
- `infer` and `warmup` run only on `InferenceWorker`'s `std::jthread`.
  GUI thread must not call them.
- One `TensorRTEngine` instance is not safe for concurrent `infer`.
- Destructor / `reset()`: context → engine → runtime → device buffers →
  stream (or equivalent RAII order that satisfies TensorRT 10 docs).
  No leaked CUDA memory or streams.

## Unchanged code (Phase 4 remainder)

`YoloPreprocess`, `YoloDecoding`, `YoloDetector`, `VisionPipeline`,
`FaceDetector`, `MotionDetector` stay as they are. Only
`createInferenceEngine` grows a `TensorRT` case when T06 lands.

## RAII types T05 must provide

| Type | Responsibility |
| --- | --- |
| `CudaStream` | create/destroy `cudaStream_t` |
| `CudaDeviceBuffer` | `cudaMalloc` / `cudaFree` + H2D/D2H on a stream |

T06 may keep TensorRT objects in `unique_ptr` with custom deleters
(`TrtRuntime`, `TrtEngine`, `TrtContext`) inside `TensorRTEngine::Impl`.
Those deleters are allowed in the engine `.cpp`; they are not a public API.

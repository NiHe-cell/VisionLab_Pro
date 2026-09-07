# Build (VisionLab Pro)

CPU-first C++20 project. Production code uses MSVC on Windows with the
official OpenCV MSVC package. MinGW cannot link that OpenCV `vc16` tree
(CMake stops with `FATAL_ERROR`).

Sanitizers and extra warnings are **opt-in**. The default configure matches
the existing `build/dev-debug` layout: TensorRT off, warnings off.

## Prerequisites

- CMake 3.21+ if you use `CMakePresets.json` (project `cmake_minimum_required`
  remains 3.16 for Qt Creator kits).
- Qt 6.8+ (`qt_standard_project_setup(REQUIRES 6.8)`), including Sql, Quick, Qml.
- OpenCV (directory that contains `OpenCVConfig.cmake`).
- ONNX Runtime official CPU zip 1.17+ (`include/onnxruntime_cxx_api.h` and `lib/`).
- Optional: CUDA Toolkit + TensorRT 10 when `VISIONLAB_ENABLE_TENSORRT=ON`.

Do not hard-code drive letters in CMake files. Pass cache variables or
environment variables.

## Configure

Presets (no machine-specific paths):

```
cmake --preset dev-debug
cmake --build --preset dev-debug
```

`dev-debug` writes to `build/dev-debug` and sets `VISIONLAB_ENABLE_TENSORRT=OFF`.
`dev-debug-trt` turns TensorRT on; you still must pass `TensorRT_DIR` and
`CUDAToolkit_ROOT` yourself.

Without presets:

```
cmake -B build/dev-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
  -DOpenCV_DIR=<opencv>/build/x64/vc16/lib ^
  -DOnnxRuntime_DIR=<onnxruntime-win-x64> ^
  -DVISIONLAB_ENABLE_TENSORRT=OFF
cmake --build build/dev-debug
```

Point CMake at Qt with `CMAKE_PREFIX_PATH` or `Qt6_DIR` (same as Qt Creator).

### Cache / environment

| Variable | Role |
|---|---|
| `OpenCV_DIR` | Directory with `OpenCVConfig.cmake` (cache or env). |
| `OnnxRuntime_DIR` | Extracted ORT CPU (or GPU) package root. |
| `VISIONLAB_ENABLE_TENSORRT` | Default OFF. ON requires CUDA + TensorRT 10. |
| `TensorRT_DIR` | TensorRT 10 root (`include/NvInfer.h`). |
| `CUDAToolkit_ROOT` | CUDA toolkit root. |
| `VISIONLAB_ENABLE_WARNINGS` | Default OFF. ON → MSVC `/W4` or `-Wall -Wextra`. Not `-Werror`. |
| `BUILD_TESTING` | From `include(CTest)`; default ON. `-DBUILD_TESTING=OFF` skips tests. |

Runtime (not CMake):

| Variable | Role |
|---|---|
| `VISIONLAB_INFERENCE_BACKEND` | `onnx-cpu` (default), `onnx-cuda`, `tensorrt`. |
| `VISIONLAB_INFERENCE_PRECISION` | `fp32` (default), `fp16` (TensorRT). |
| `VISIONLAB_INFERENCE_DEVICE` | GPU index, default `0`. |
| `VISIONLAB_PLUGIN_DIR` | Plugin scan directory; empty → `<app>/plugins`. |

Invalid inference env values warn and fall back to CPU / fp32 / 0. A GPU
backend that fails to initialize does **not** silently switch to CPU.

## Tests

```
ctest --test-dir build/dev-debug -L cpu --output-on-failure
```

See `docs/testing.md` for labels and `QSKIP` policy. Missing
`yolov4-tiny.onnx` should skip ORT model slots, not invent timings.

## Sanitizers (not default)

Do not turn AddressSanitizer / UndefinedBehaviorSanitizer on in the MSVC + Qt
default tree. If you need them, use a **separate** Clang or GCC build directory
and follow that compiler's docs (`-fsanitize=address,undefined`).
ThreadSanitizer + Qt is not a supported default combination for V1.

## Benchmarks

`bench_onnx_cpu`, `bench_pipeline`, `bench_tracker` are built with the project.
They are not latency gates in `ctest`. Commands: `docs/benchmarks.md`.

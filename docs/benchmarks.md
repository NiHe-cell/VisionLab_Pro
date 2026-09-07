# Benchmarks

These binaries print metric **field names**. Values change with CPU, Debug vs
Release, and load. This file does not contain measured FPS or latency tables
(see `docs/performance.md`). Do not copy the
Performance page 250 ms snapshot here.

## Inference — `bench_onnx_cpu`

Compare backends with **separate processes** (do not chain four backends in
one process):

```
bench_onnx_cpu --backend onnx-cpu --model <path.onnx>
bench_onnx_cpu --backend onnx-cuda --model <path.onnx>
bench_onnx_cpu --backend tensorrt --precision fp32 --model <path.onnx>
bench_onnx_cpu --backend tensorrt --precision fp16 --model <path.onnx>
```

CLI tests (`onnxcpubench`) only check exit codes and key names.

## Pipeline overload — `bench_pipeline`

Fake looping source + `SlowDetector`. No camera, no YOLO.

```
bench_pipeline --seconds 8 --queue 2 --detect-ms 50 --width 64 --height 64
```

Stdout keys: `backend`, `seconds`, `queue_capacity`, `detect_ms`, `captured`,
`processed`, `dropped`, `peak_queue_depth`, `e2e_p50_ms`, `e2e_p95_ms`,
`still_running_before_stop`.

`ctest` target `pipelinebench` does not assert `dropped > 0` (fast machines
would flake). DropOldest behavior stays in `pipelineoverload`.

### Soak (local only)

Zero-delay detect uses `FakeDetector` instead of sleeping:

```
bench_pipeline --seconds 3600 --detect-ms 0
```

Watch whether the process stays alive, whether `dropped` stays bounded, and
whether working-set memory in Task Manager trends without bound. This is an
**operator procedure**, not a proof of no leaks (no ASan in the default MSVC
build). Do not write a megabyte cap unless you measured it on a named machine.

CI must not pass `--seconds` greater than 15. GPU memory is not measured here;
use `bench_onnx_cpu` `gpu_mem_mb` on a CUDA/TensorRT process.

## Tracker — `bench_tracker`

Synthetic boxes through `ByteTrackTracker::update`. No images, no ONNX.

```
bench_tracker --frames 200 --dets 10 --warmup 10
```

Stdout keys: `backend`, `frames`, `dets`, `warmup`, `mean_ms`, `p50_ms`,
`p95_ms`, `active_tracks`. `trackerbench` only checks keys and exit codes.

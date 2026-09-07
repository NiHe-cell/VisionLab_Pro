# Performance measurement templates

Fill these tables from **this machine's** benchmark stdout. Empty cells stay
`[NOT MEASURED]`. Do not paste UI Performance-page snapshots (250 ms timer)
or numbers from chat history.

Record the environment next to every filled row: OS, CPU, GPU, driver, ORT
version, TensorRT version, CMake build type (`Debug` or `Release`).

How to fill inference rows: run one backend per process, copy keys from
`bench_onnx_cpu` stdout (`docs/benchmarks.md`). Do not chain four backends
in one process.

## Inference backends

| Backend | Precision | Model | Input | mean_ms | P50 | P95 | P99 | throughput_fps | GPU Memory | Notes |
|---|---|---|---|---|---|---|---|---|---|---|
| ORT CPU | FP32 | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | |
| ORT CUDA | FP32 | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | |
| TensorRT | FP32 | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | |
| TensorRT | FP16 | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | |

Machine / build: `[NOT MEASURED]`

## Pipeline overload (`bench_pipeline`)

Stdout keys: `backend`, `seconds`, `queue_capacity`, `detect_ms`, `captured`,
`processed`, `dropped`, `peak_queue_depth`, `e2e_p50_ms`, `e2e_p95_ms`,
`still_running_before_stop`.

Command example:

```
bench_pipeline --seconds 8 --queue 2 --detect-ms 50 --width 64 --height 64
```

| backend | seconds | queue_capacity | detect_ms | captured | processed | dropped | peak_queue_depth | e2e_p50_ms | e2e_p95_ms | still_running_before_stop | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| fake-pipeline | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | |

Machine / build: `[NOT MEASURED]`

CI may run `--seconds 2` as a crash smoke. That run is not a latency result.

## Tracker (`bench_tracker`)

Stdout keys: `backend`, `frames`, `dets`, `warmup`, `mean_ms`, `p50_ms`,
`p95_ms`, `active_tracks`.

```
bench_tracker --frames 200 --dets 10 --warmup 10
```

| backend | frames | dets | warmup | mean_ms | p50_ms | p95_ms | active_tracks | Notes |
|---|---|---|---|---|---|---|---|---|
| bytetrack | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | [NOT MEASURED] | |

Machine / build: `[NOT MEASURED]`

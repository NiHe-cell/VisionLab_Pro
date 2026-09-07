# Resume evidence (placeholders until measured)

Numeric claims stay in brackets until a named machine fills
`docs/performance.md`. Do not replace brackets with remembered FPS.

## Latency / throughput (fill after measurement)

- Reduced P95 from `[BASELINE]` ms to `[FINAL]` ms (`bench_onnx_cpu`, backend
  `[BACKEND]`, model `[MODEL]`, build `[Debug|Release]`, machine `[HOST]`).
- Pipeline overload: `dropped` `[BASELINE]` → `[FINAL]` at
  `queue_capacity=[N]`, `detect_ms=[T]` (`bench_pipeline` stdout keys).
- Tracker `p95_ms` `[BASELINE]` → `[FINAL]` (`bench_tracker`, `frames` /
  `dets` recorded in the table).

## Verifiable non-numeric work (this repository)

- Four detector plugins: `vision.dummy`, `vision.motion`, `vision.face`,
  `vision.yolo` (`plugin/`, `plugins/`).
- Four analytics rules (ROI, line, loiter, count) behind `IRule` / `RuleSpec`.
- Three inference backend abstractions: ORT CPU, ORT CUDA, TensorRT 10
  (`IInferenceEngine` / `createInferenceEngine`). INT8 is not in V1.
- SQLite event store with `prune(10000)` after insert (`EventWriter`,
  `SqliteEventRepository`).
- Real-time path: `BoundedQueue<FramePacket>` DropOldest, `std::jthread`
  capture + inference, `EventWriter` `jthread`, no inference on the GUI thread.
- Four-page QML shell: Monitor, Events, Performance, Settings; rule and
  inference edits apply only while stopped.
- CPU CTest label `cpu` (65 tests in the current tree) plus optional `gpu`
  tests when TensorRT is enabled.

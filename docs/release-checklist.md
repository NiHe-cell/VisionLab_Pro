# V1.0 release checklist

Phase 9 Definition of Done (`docs/cursor/phase-prompts.md`,
`docs/cursor/task-decomposition.md`). Check a box only with evidence.
Do not check long-running soak unless an operator actually ran it.
Do not check GitHub Actions green until the Actions tab shows a pass.

## CPU build

- [x] Local CPU-capable tree builds and links (this session ran the existing
      `build/dev-debug` test binaries).
- [ ] GitHub Actions `windows-cpu` job passed (see badge / Actions). **Not
      claimed in this session.**

## Tests

- [x] `ctest -L cpu --output-on-failure` this session (2026-09-07, Windows
      MSVC, `build/dev-debug`): **65 passed, 0 failed**, CTest real time
      25.29 s. No CTest-level skips listed. Internal Qt `QSKIP` still counts
      as Passed at this layer.
- [ ] `ctest -L gpu` this session: **[NOT RUN THIS SESSION]**.

## Pipeline

- [x] Bounded `BoundedQueue<FramePacket>` + DropOldest (`pipelineoverload`,
      `pipelinelifecycle`).
- [x] Capture / inference `std::jthread`; GUI does not call `detect` /
      `infer`.
- [ ] Hour-scale soak (`bench_pipeline --seconds 3600`): **not run**; do not
      mark stable.

## ONNX CPU

- [x] `OnnxRuntimeEngine` + `onnxruntimecudaengine` **cpu** label (CUDA
      identity path may `QSKIP` without a device).
- [x] `bench_onnx_cpu` CLI exists (`onnxcpubench` checks keys, not ms).

## CUDA / TensorRT

- [ ] CUDA EP end-to-end on this machine: **N/A — not run this session**.
- [ ] TensorRT `ctest -L gpu`: **N/A — not run this session**.
- [x] Code/docs present; CI does not install CUDA.

## Plugins / tracking / rules / UI / SQLite

- [x] Plugins: dummy, motion, face, yolo (`pluginmanager`, plugin tests).
- [x] ByteTrack (`bytetracktracker`).
- [x] Four rules + `RuleSpec` stopped apply (`rulespec`, rule tests).
- [x] QML four pages (`views/Main.qml`).
- [x] SQLite writer + prune 10000 (`eventrepository`, `eventwriter`).

## Benchmarks reproducible

- [x] `docs/benchmarks.md` commands and stdout keys.
- [x] `docs/performance.md` tables default `[NOT MEASURED]`.
- [ ] Filled inference/pipeline/tracker numbers: **not in this close-out**.

## Documentation / attribution / honesty

- [x] Architecture diagrams: `docs/architecture.md`.
- [x] Build + CI notes: `docs/build.md`.
- [x] Interview notes: `docs/interview-notes.md`.
- [x] Architecture review: `docs/architecture-review.md`.
- [x] LICENSE keeps Muhammed Suwaneh; adds VisionLab Pro / NiHe-cell.
- [x] README clone URL is `https://github.com/NiHe-cell/VisionLab_Pro.git`.
- [x] No invented FPS/latency in docs added this phase.
- [x] qmltestrunner: **explicitly not done** (Future).

## V1.0 mark

Local CPU tests in this session passed. Treat GitHub `master` as V1.0
**documentation-complete** after this checklist’s code/docs items. Treat the
public CI badge as informational until Actions is green.

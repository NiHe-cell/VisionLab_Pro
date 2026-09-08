# Architecture review (V1.0, HEAD)

Review of the tree as it stands after Phases 0–9. Scope checklist is the 30
items in `docs/cursor/code-review.md`. This file records **status**, not a
new implementation task.

**Blockers for V1.0:** none observed in this session.

Blocker would mean: default CPU configure/build broken, inference on the GUI
thread, unbounded frame queue, or an untested data race on the hot path.
Local `ctest -L cpu` on 2026-09-07: **65 passed, 0 failed** (CTest summary;
Qt `QSKIP` inside a passing binary is not a CTest skip). GitHub Actions is
not treated as green here until a recorded run passes.

## 30-item status (code-review.md)

| # | Topic | V1 status |
|---|---|---|
| 1 | Ownership | `unique_ptr` pipeline / plugins / engines. Queue and latest-result are pipeline-owned. |
| 2 | Lifetime | Detectors destroyed before `QPluginLoader`. Writer after pipeline on process exit. |
| 3 | Dangling | Workers join before queue reset. Plugin scan only while stopped. |
| 4 | `cv::Mat` | Inference clones before draw; GUI `QImage::copy` from `PresentedFrame.rgb`. |
| 5 | Data races | Queue / LatestResult / EventLog / StatsProbe mutexes. `RuleEngine` not shared with GUI while running. |
| 6–7 | Deadlock / lock order | No nested lock protocol beyond single mutexes on those mailboxes. |
| 8–9 | GUI thread / affinity | Detect/infer not on GUI. Models and `CameraManager` are GUI `QObject`s. |
| 10 | Shutdown | `joinWorkers` + writer `stop`. EventWriter `jthread` exits via queue `close`, not `stop_token`. |
| 11–12 | Leaks / CUDA | ORT sessions RAII. TensorRT path uses CUDA RAII wrappers when enabled. |
| 13–14 | Exceptions / RAII | Worker catches `cv::Exception` per stage. Queues and jthreads are RAII. |
| 15 | `shared_ptr` | Not the ownership model for detectors / engines. |
| 16–17 | Copies | Frame clone before overlay is intentional. LatestResult snapshot copies `PresentedFrame`. |
| 18–21 | Coupling | Detectors do not draw. Backends stay behind `IInferenceEngine`. QML does not include `pipeline/`. |
| 22 | Plugin lifetime | No unload (documented). |
| 23 | State machine | Settings / rules apply only while stopped. |
| 24 | Tests | `cpu` label covers queue, workers, plugins, rules, SQLite, benches CLI. |
| 25 | Unrelated edits | Phase 9 docs/CI only in this close-out. |
| 26 | API | `applyRuleSpecs` / `applySessionSettings` fail closed when running. |
| 27 | Performance | No fake gates. UI 250 ms vs bench processes. |
| 28 | Build | Presets; MinGW + official OpenCV `FATAL_ERROR`. |
| 29–30 | Errors / logs | GPU failure does not CPU-fallback. Invalid env warns. |

## Blocker

None.

## High

None that fail the V1.0 definition in `docs/cursor/phase-prompts.md` Phase 9
DoD. Remaining items below are accepted limits or Future work.

## Medium

| Location | Phenomenon | Blocks V1.0? |
|---|---|---|
| `pipeline/EventLog.h` (capacity 256); `CameraManager::notifyFrame` | If the GUI stalls, DropOldest drops events before `EventWriter` enqueue. SQLite is not a full trace. `docs/storage/events.md`. | No |
| `analytics/RuleEngine` | `evaluate` / `addRule` are not thread-safe; no hot reload. Mutations only while stopped. | No (by contract) |
| `plugin/PluginManager.h` | No `unload()`. Process restart to swap DLLs. | No |
| `views/` vs `screenshots/` | Historical PNGs show the old single-page UI. | No |
| `inference/` line endings | Working-tree CRLF noise on some files. Do not mass-reformat in a docs commit. | No |

## Low

| Location | Phenomenon | Blocks V1.0? |
|---|---|---|
| `CMakeLists.txt` `VISIONLAB_ENABLE_WARNINGS` | Default OFF; not `-Werror`. | No |
| Default sanitizers | MSVC + Qt tree does not enable ASan/UBSan. | No |
| `EventWriter` `jthread` lambda | Ignores `stop_token`; shutdown is `queue->close()`. | No |
| Title bar `PreserveAspectCrop` | Logo only; video plane is `PreserveAspectFit`. | No |

## Future

| Item | Notes |
|---|---|
| INT8 | Explicitly out of TensorRT V1 contract. |
| qmltestrunner | Not in the test inventory; C++ tests cover models. |
| Plugin hot-reload | Would need detector destroy + unload order. |
| Rule mutate while running | Would need a thread-safe swap or a rule thread. |
| Larger EventLog / persist-on-push | Close the GUI-stall window. |
| Rename `models/` | Qt `*Model` sources share a folder with Caffe/ONNX files. |
| Screenshot refresh | Night Ops QML is in tree; `screenshots/*.png` are still upstream. |
| CI CUDA/TensorRT job | Not in `.github/workflows/ci.yml`. |

## Technical debt (do not “fix” inside a review commit)

- Mixed `models/` directory.
- `inference/` CRLF.
- EventLog 256 vs prune-10000 SQLite window.
- No default sanitizer / qmltestrunner / INT8 / plugin unload.

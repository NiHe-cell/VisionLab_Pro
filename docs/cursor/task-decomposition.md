We are about to start the current VisionLab Pro development phase.

Do not write code yet.

Based on:

1. The current repository state.
2. Changes already completed in previous phases.
3. The goals and Definition of Done of the current phase.

Analyze the codebase and break this phase into small implementation tasks.

Requirements for task decomposition:

* Each task should have one primary engineering responsibility.
* Prefer tasks that can be independently compiled and tested.
* Avoid repository-wide changes in a single task.
* Explicitly identify dependencies between tasks.
* Identify which tasks change public interfaces.
* Identify which tasks introduce concurrency risk.
* Identify which tasks require new third-party dependencies.
* Identify which tasks require new tests.
* Identify which tasks require benchmark measurements.

For every task provide:

Task ID

Task Name

Goal

Files likely affected

New files

Interfaces affected

Implementation outline

Risks

Required tests

Acceptance criteria

Recommended Git commit message

Finally produce the recommended execution order.

Do not implement anything until I explicitly tell you which Task ID to execute.

After a Task ID is implemented, append the completion record to the sole living
report `%USERPROFILE%\Desktop\VisionLab_Pro任务总结报告.docx`. Do not create
another report file. See `.cursor/rules/task-summary-report.mdc`.

---

# Phase 9 — Benchmark、测试工程化、CI、文档与 V1.0 收口 任务分解

对照 HEAD `6142504`（`feat(ui): add performance and settings pages with stopped-pipeline apply`）。
Phase 0–8 实现已提交并推到 `origin/master`。Phase 8 未做单独「收口」提交，但不阻塞本阶段：
四页壳、Letterbox、RuleSpec、SQLite EventWriter、Settings 停机 Apply 都在 `master` 上。

本阶段**不增加产品功能**。不做：规则 JSON 落盘、事件快照 JPEG、规则热更新、INT8、
独立 RuleWorker、qmltestrunner 全量套件、Material 换肤、Linux 首次移植。
不把工作区里的 `inference/` 换行噪声、未跟踪 `.cursor/` 卷进提交。
`AGENTS.md` 工作区补丁（任务报告约定）可在 T07 一并提交；不要和基准代码混在一个 commit。

已有、不要重做：

- `bench_onnx_cpu`（别名 `bench_inference`）：ORT CPU / CUDA / TensorRT 四进程对比；
  `tst_onnxcpubench` 只断言缺模型退出码与 identity 夹具**字段名**，不设延迟门槛。
- `tst_pipelineoverload`：DropOldest 有界、会丢帧；不是可复现的基准二进制。
- 领域单测已覆盖 BoundedQueue、预处理/后处理、管线 start/stop、插件、ByteTrack、
  四条规则、SQLite、EventWriter、各 QAbstractListModel。
- `VISIONLAB_ENABLE_TENSORRT` 默认 OFF；GPU 测试用 `QSKIP`。

## 当前架构

```
QML (Main / SideNav / Monitor / Events / Performance / Settings)
  → VisionController（模型 + 250ms 性能定时器 + 停机 Apply）
      → CameraManager（GUI）
          PluginManager → IVisionPlugin → unique_ptr<IDetector>
          VisionPipeline
            CaptureWorker (jthread)
              → BoundedQueue<FramePacket>  DropOldest
            InferenceWorker (jthread)
              IDetector::detect
              ITracker::update            （可关：tracker 空）
              RuleEngine::evaluate        （停机注入 RuleSpec）
              TrackRenderer / DetectionRenderer
              → LatestResult<PresentedFrame>
              → EventLog（256，DropOldest）
          EventWriter (jthread) → SQLite
          CameraImageProvider → QML RGB
```

CMake 库：`visionlab_core`（无 Qt）→ video / inference / detectors / plugin /
tracking / analytics / rendering / pipeline / storage。app 链 Quick + Sql。
远程：`origin` = `NiHe-cell/VisionLab_Pro`，`upstream` = `Muhammedsuwaneh/vision-lab`。
README 仍是上游演示文案（C++17、`std::thread`、clone 上游 URL）。无 `.github/`、
无 `CMakePresets.json`、无 `docs/interview-notes.md`。默认编译只有 `/utf-8`，无 `/W4`。

## 锁定的设计决策

1. **不编造数字。** 基准工具只打印字段。`docs/performance.md` 表格空单元格写
   `[NOT MEASURED]`，直到某次真实运行把 stdout 贴进去。禁止把 Performance 页
   250ms 快照抄进报告当「保证 FPS」。
2. **默认 `ctest` 不是性能门槛。** 新 bench 可执行文件不 `add_test` 去比较
   mean_ms / FPS。配套 Qt Test 只查：非法参数非 0、成功路径 stdout 含约定键、
   进程能停。
3. **三个基准二进制：** 推理沿用 `bench_onnx_cpu`；新增 `bench_pipeline`、
   `bench_tracker`。不要再做一个「四后端单进程连跑」。
4. **管线基准用假源。** `FakeVideoSource`（可 loop）+ `SlowDetector`，不打开摄像头、
   不跑 YOLO。默认模拟「生产快、推理慢」。打印
   `captured` / `processed` / `dropped` / `peak_queue_depth` / `e2e_p50_ms` /
   `e2e_p95_ms` / `seconds`。可关 DropOldest 不在本阶段（队列策略已锁）。
5. **Soak 不是 CI 小时任务。** `bench_pipeline --seconds N`（默认 8）。
   文档写明本地可 `--seconds 3600`。CI 最多跑默认秒数。不测 GPU 显存曲线。
6. **跟踪基准纯合成 Detection。** 不读图。每帧固定 N 个框，跑 M 帧
   `ByteTrackTracker::update`，打印 `mean_ms` / `p50_ms` / `p95_ms` / `tracks`。
7. **CTest labels：** 现有测试标 `cpu`；仅 TensorRT/CUDA RAII 标 `gpu`（已在
   `if(VISIONLAB_ENABLE_TENSORRT)` 里）。`onnxruntimecudaengine` 标 `cpu`
   （无 GPU 时 round-trip `QSKIP`，构造/失败路径仍要在 CPU CI 跑）。
8. **警告与 sanitizer 不进默认。** `option(VISIONLAB_ENABLE_WARNINGS OFF)`：
   ON 时 MSVC `/W4`、非 MSVC `-Wall -Wextra`，**不加 `/WX` / `-Werror`**。
   ASan/UBSan/TSan 只写在 `docs/build.md`，MSVC+Qt 默认构建不强制。
9. **CI 只保证 CPU。** `.github/workflows/ci.yml`：`windows-2022`，
   `VISIONLAB_ENABLE_TENSORRT=OFF`，`ctest -L cpu`。Qt 用 `install-qt-action`
   安装 **6.8.3** `win64_msvc2022_64`（与 `qt_standard_project_setup(REQUIRES 6.8)`
   对齐）。ORT 用官方 **CPU** zip（1.17+，文档钉具体 tag）。OpenCV 用官方
   Windows pack 的 `OpenCVConfig.cmake`。缺模型文件时现有 `QSKIP` 必须仍绿。
   不在 CI 装 CUDA/TensorRT。不宣称 Linux 构建。
10. **文档 DRY。** Mermaid 只放 `docs/architecture.md`；README 链过去，不复制两份会漂的图。
11. **许可与归属。** `LICENSE` 保留 MIT 与上游 Copyright；另加 VisionLab Pro 贡献者行。
    README 写明：架构级二次开发，基于 `Muhammedsuwaneh/vision-lab`。
12. **qmltestrunner 本阶段不做。** UI 契约靠 `tst_visioncontroller` / 模型单测 /
    `docs/ui/qml-boundary.md`。列入 T10 的 Future，不阻塞 V1.0。
13. **本阶段不改** ByteTrack / 四条规则状态机 / InferenceWorker 热路径 /
    QML 视觉样式。CMake 只加 bench 目标、option、presets、labels。

## 推荐执行顺序

`P9-T01 → T02 → T03 → T04 → T05 → T06 → T07 → T08 → T09 → T10`

T02 与 T03 无互相依赖，T01 之后可并行。T04 依赖 T02 的 `bench_pipeline` CLI。
T06 依赖 T05 的 presets / labels。T07–T10 是文档，T08 的图供 T07 链接；
T09 的性能表在有真实跑数前保持 `[NOT MEASURED]`。T10 最后做，避免评审过期。

---

### P9-T01  测试清单与 CTest labels

**Task ID:** P9-T01

**Task Name:** 测试审计文档 + cpu/gpu 标签

**Goal:** 把现有 `add_test` 对照 Phase 9 覆盖面写成可维护清单；给 CI 提供 `-L cpu`。
不新写产品逻辑。不为「覆盖率百分比」补空洞测试。

**Files likely affected:**
- `tests/CMakeLists.txt`（每个 `add_test` 设 `LABELS`）

**New files:**
- `docs/testing.md`

**Interfaces affected:** 无 C++ API。CTest 属性：`cpu` / `gpu`。

**Implementation outline:**
- `docs/testing.md` 用表列出：区域 → 测试名 → 文件 → 缺模型/无 GPU 时行为（跑 / `QSKIP`）。
  至少覆盖：BoundedQueue、LatestResult、preprocess、postprocess、IInferenceEngine、
  factory、管线 lifecycle/overload、PluginManager、ByteTrack、四规则、RuleEngine、
  SQLite、EventWriter、各 Model、CameraManager、VisionController、letterbox。
- 明确「高风险已覆盖、本任务不补测」若审计后没有空洞。
- 若发现**零覆盖**的高风险缺口（审计时对照源码），只补那一个测试文件。
  当前仓库对照结果：上列区域均已有 `add_test`，**预期本任务零新 tst_***。
- labels：`if(VISIONLAB_ENABLE_TENSORRT)` 下的 `tensorrtengine` /
  `tensorrtenginecache` / `cudaraii` → `gpu`；其余 → `cpu`。
- `onnxcpubench` 标 `cpu`（只跑 CLI 字段，identity 夹具在 `tests/fixtures/`）。

**Risks:**
- 漏标导致 CI `ctest -L cpu` 漏掉 cameramanager。
- 给 `onnxruntimecudaengine` 标 `gpu` 会让 CPU CI 失去失败路径覆盖。

**Required tests:**
- 本地：`ctest -N -L cpu` 列出的名字包含 `boundedqueue`、`visionpipeline`、
  `eventrepository`、`bytetracktracker`、`cameramanager`。
- `ctest -N -L gpu`：TensorRT OFF 时为空；ON 时含三个 gpu 测试。
- 不跑全量计时。

**Acceptance criteria:**
- 文档不出现编造的通过率或 FPS。
- 生产二进制行为不变。

**Dependencies:** 无。接口变更：否（CTest 属性）。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `docs(test): inventory coverage and label cpu vs gpu ctests`

---

### P9-T02  管线过载基准 `bench_pipeline`

**Task ID:** P9-T02

**Task Name:** DropOldest 管线基准工具

**Goal:** 可复现地打印过载下队列深度与丢帧。证明「慢推理 + 有界队列」不会让内存随
摄像头无限涨。不打开真实摄像头。

**Files likely affected:**
- 根 `CMakeLists.txt`：`add_executable(bench_pipeline ...)`，链 `visionlab_pipeline`
- `tests/CMakeLists.txt`：`tst_pipelinebench`；Windows PATH 列表加上 `pipelinebench`
- `docs/testing.md` 或本任务先写 `docs/benchmarks.md` 骨架（T04 补 soak 段）

**New files:**
- `benchmarks/PipelineBenchmark.cpp`
- `tests/PipelineBenchmarkTest.cpp`
- `docs/benchmarks.md`（CLI；数字表留给 T09）

**Interfaces affected:** 无库 API。CLI：

```
bench_pipeline [--seconds N] [--queue N] [--detect-ms M] [--width W] [--height H]
```

默认：`seconds=8`，`queue=2`（与 `VisionPipeline::kDefaultQueueCapacity` 一致），
`detect-ms=50`，`width=64`，`height=64`。

成功 stdout（键名锁死，便于测试与贴表）：

```
backend: fake-pipeline
seconds: ...
queue_capacity: ...
detect_ms: ...
captured: ...
processed: ...
dropped: ...
peak_queue_depth: ...
e2e_p50_ms: ...
e2e_p95_ms: ...
```

非法 `--seconds` / 非数字 → 退出码 2，无上述计量行。`--help` 退出 0。

**Implementation outline:**
- 把 `tests/fakes/FakeVideoSource.h`、`SlowDetector.h` 的实现抽到
  `benchmarks/` **不要**。基准 cpp **可 include** `tests/fakes/...`（测试头已是
  header-only），`target_include_directories` 加上 `${CMAKE_SOURCE_DIR}/tests`。
  不要把 Fake 搬进 `video/` 生产库。
- `FakeVideoSource(..., loop=true)` + `SlowDetector(detect-ms)` + `VisionPipeline`。
- 跑 N 秒后 `stop()`，读 `pipeline.stats()`。`peak_queue_depth` 在循环里
  `max(stats.captureQueueDepth)`。
- `e2e_*` 用 `PipelineStats::endToEndLatencyMs` 不够（那是瞬时/平均窗口）——
  **锁定：在循环内对 `stats().endToEndLatencyMs` 采样进本地 `LatencyWindow`，**
  结束时打 p50/p95。不要改 `StatsProbe` 公共接口。
- 不 `add_test` 去断言 `dropped > 0`（机器快慢会导致假红）。行为锁在
  `tst_pipelineoverload`。本任务测试只查 CLI。

**Risks:**
- 把 YOLO / 摄像头塞进 bench → CI 不可跑。
- 断言 `dropped>=1` → 快机器假红。
- 忘了 `stop()` / join → 测试挂起。

**Required tests:**
- `--help` 退出 0。
- `--seconds -1` 或 `--queue 0` → 非 0，stdout/stderr 无 `dropped:`。
- 默认参数（可在测试里传 `--seconds 2 --detect-ms 50 --queue 2`）退出 0，
  输出含全部键名。`QVERIFY(waitForFinished)` 超时 ≤ 30s。不比较数值大小。

**Acceptance criteria:**
- 不链 Qt Quick。不写延迟门槛进 ctest。
- 文档声明：数字随机器变，本文件不填表。

**Dependencies:** T01 仅文档交叉链接，不强制。接口变更：否。并发风险：是（jthread，须 stop）。
新第三方依赖：否。需测量：否（打印但不把结果当门槛）。

**Recommended Git commit message:** `bench(pipeline): add DropOldest overload harness with fake source`

---

### P9-T03  跟踪基准 `bench_tracker`

**Task ID:** P9-T03

**Task Name:** ByteTrack 合成负载计时

**Goal:** 不依赖摄像头/GPU，测量 `ITracker::update` 延迟分布。

**Files likely affected:**
- 根 `CMakeLists.txt`：`bench_tracker` 链 `visionlab_tracking`
- `tests/CMakeLists.txt`：`tst_trackerbench`；PATH 加上 `trackerbench`

**New files:**
- `benchmarks/TrackerBenchmark.cpp`
- `tests/TrackerBenchmarkTest.cpp`

**Interfaces affected:** 无库 API。CLI：

```
bench_tracker [--frames N] [--dets K] [--warmup N]
```

默认：`frames=200`，`dets=10`，`warmup=10`。

成功 stdout：

```
backend: bytetrack
frames: ...
dets: ...
warmup: ...
mean_ms: ...
p50_ms: ...
p95_ms: ...
active_tracks: ...
```

**Implementation outline:**
- 每帧生成 K 个 `Detection`（框在 640×360 内缓慢平移，classId=0），调用
  `ByteTrackTracker::update`。用 `LatencyWindow` 记录每次 update 的毫秒。
- 非法参数退出 2。缺未知 flag 退出 2。
- 不读 ONNX，不链 inference。

**Risks:**
- 把真实视频解码塞进来 → 不可移植。
- ctest 比较 mean_ms → 禁止。

**Required tests:**
- 未知参数非 0，无 `mean_ms:`。
- `--frames 5 --dets 2 --warmup 1` 退出 0，含全部键。超时 ≤ 15s。

**Acceptance criteria:**
- `visionlab_tracking` 源码零算法改动。
- 不把结果写入 `docs/performance.md`（留给人工跑 T09）。

**Dependencies:** 无。接口变更：否。并发风险：否（单线程）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `bench(tracking): time ByteTrack update on synthetic detections`

---

### P9-T04  短 soak 模式与稳定性说明

**Task ID:** P9-T04

**Task Name:** `bench_pipeline --seconds` 文档化 soak

**Goal:** 给出可本地拉长的稳定性跑法；CI 只跑短秒数。不在仓库里提交小时级日志。

**Files likely affected:**
- `benchmarks/PipelineBenchmark.cpp`（T02 已有 `--seconds`；本任务补 `--detect-ms 0`
  表示 FakeDetector 无睡眠，用于 soak 而非过载）
- `docs/benchmarks.md`
- `tests/PipelineBenchmarkTest.cpp`（`--detect-ms 0 --seconds 2` 仍退出 0）

**New files:** 无（或仅文档小节）

**Interfaces affected:** `--detect-ms 0`：用 header-only `FakeDetector` 替换 `SlowDetector`。
仍 loop 假源。结束条件仍是时间到 + `stop()`。额外打印 `still_running_before_stop: 0|1`
（停之前 `isRunning()`）。

**Implementation outline:**
- 文档步骤：过载命令、soak 命令（例 `--seconds 3600 --detect-ms 0`）、观察项
  （进程是否还在、`dropped` 是否单调、任务管理器内存是否明显单边上扬）。
- **禁止**写「内存稳定 < X MB」除非附带一次真实测量（本任务不测量则不写死数字）。
- 说明：本 soak **不是** GPU 显存测试；GPU 见 `bench_onnx_cpu` 的 `gpu_mem_mb`。

**Risks:**
- 在 CI 里默认 3600 秒。默认必须保持 T02 的 8 秒；测试传 2 秒。
- 把「无泄漏」写成已证明——未跑 ASan 就只能写「操作说明」。

**Required tests:**
- `--detect-ms 0 --seconds 2` 退出 0，含 `dropped:` 键（值可为 0）。

**Acceptance criteria:**
- GitHub Actions 不调用 `--seconds` > 15。
- 无伪造泄漏结论。

**Dependencies:** T02。接口变更：CLI 小扩展。并发风险：是（须 stop）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `bench(pipeline): add zero-delay soak mode and stability instructions`

---

### P9-T05  CMake presets、可选警告、构建文档

**Task ID:** P9-T05

**Task Name:** 可重复配置与警告开关

**Goal:** 新人能按文档 + preset 配出与本机 `build/dev-debug` 同类的 CPU 构建。
不把 sanitizer 编进默认。

**Files likely affected:**
- 根 `CMakeLists.txt`：`option(VISIONLAB_ENABLE_WARNINGS OFF)`
- `tests/CMakeLists.txt`：无行为变化除非注释

**New files:**
- `CMakePresets.json`
- `docs/build.md`

**Interfaces affected:**

```cmake
option(VISIONLAB_ENABLE_WARNINGS
    "Enable extra compiler diagnostics (/W4 or -Wall -Wextra). Does not enable -Werror."
    OFF)
```

Preset 至少：

| name | 说明 |
|---|---|
| `dev-debug` | `binaryDir` = `build/dev-debug`，`VISIONLAB_ENABLE_TENSORRT=OFF` |
| `dev-debug-trt` | 同上但 TensorRT ON（cache 变量留空，本机自己填 `TensorRT_DIR`） |

`cacheVariables` **不要**写死本机 `E:\` / `D:\` 绝对路径。用 `OpenCV_DIR` /
`OnnxRuntime_DIR` 的**环境变量**说明写在 `docs/build.md`。
Qt Creator 已有的 kit 不被 presets 删除。

`docs/build.md` 必须写：

- C++20、MSVC、禁止用 MinGW 链官方 OpenCV vc16 包（已有 FATAL_ERROR）。
- `find_package` 提示：`OpenCV_DIR`、`OnnxRuntime_DIR`、可选 `VISIONLAB_ENABLE_TENSORRT`。
- 环境变量：`VISIONLAB_INFERENCE_*`、`VISIONLAB_PLUGIN_DIR`。
- ASan/UBSan：仅建议 Clang/GCC 独立构建树；MSVC+Qt 默认不做。TSan 与 Qt 同用列为不支持默认组合。
- `BUILD_TESTING` 已由 `include(CTest)` 控制。

**Implementation outline:**
- warnings ON 时 `add_compile_options` 包在 option 里，不影响现网默认绿构建。
- 不改 `qt_standard_project_setup` 版本下限。

**Risks:**
- Preset 写死本机路径 → 别人 configure 失败，也泄漏盘符。
- 默认打开 `/W4` → 日志洪水，面试 clone 体验差。

**Required tests:**
- 默认（warnings OFF）现有 `ctest -L cpu` 仍能配置（本任务以 configure 成功为准；
  全量 ctest 可放在 T06 前本地跑一次）。
- 不要求在本任务打开 warnings 并清零告警。

**Acceptance criteria:**
- `docs/build.md` 无绝对 Windows 用户路径。
- 默认构建选项与 HEAD 相比仅多一个 OFF option。

**Dependencies:** 无。接口变更：CMake option。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `build: add CMake presets, optional warnings, and build notes`

---

### P9-T06  GitHub Actions CPU CI

**Task ID:** P9-T06

**Task Name:** 无 GPU 的 configure / build / ctest

**Goal:** 在 GitHub 上证明 CPU 树能编过、`ctest -L cpu` 能跑。GPU 作业不做。

**Files likely affected:**
- `docs/build.md`（加 CI 段：与本地差异、缓存、模型缺失则 QSKIP）

**New files:**
- `.github/workflows/ci.yml`

**Interfaces affected:** 无 C++。Workflow 锁：

- `on: push` / `pull_request`，路径可不过滤（仓库不大）。
- `runs-on: windows-2022`。
- 环境：`VISIONLAB_ENABLE_TENSORRT=OFF`。
- 步骤顺序：checkout → 装 Qt 6.8.3 MSVC 64 → 还原/下载 OpenCV Windows pack →
  还原/下载 ORT **CPU** zip（1.17+，在 yml 注释里钉 **完整文件名**，例如
  `onnxruntime-win-x64-1.17.3`；实现时查 GitHub ORT release，不要编造不存在的 tag）→
  `cmake --preset dev-debug` 或等价 `-B build/ci` 传入 `OpenCV_DIR` / `OnnxRuntime_DIR` →
  `cmake --build` → `ctest --test-dir ... -L cpu --output-on-failure -C Debug`。
- 缓存：`actions/cache` 键包含 Qt 版本、OpenCV 版本、ORT 版本。
- `yolov4-tiny.onnx` / caffemodel **不**从网上下到 CI（仓库也未跟踪 onnx）。
  依赖 `QSKIP` 的测试必须继续 skip 而不是 fail。
- `fail-fast: false` 可选；不要 `continue-on-error` 掩盖编译失败。

**Implementation outline:**
- Qt：`jurplel/install-qt-action`（或当前仍维护的等价 action），modules 至少
  `qtbase` `qtdeclarative`（Quick/Qml）以及 **Sql**（`tst_eventrepository`）。
- 若某依赖官方 URL 不稳定：workflow 里用 `env` 变量写 URL，失败时日志打印
  「需要维护者更新镜像」，**不要**用空 job 假装绿。
- 不在 CI 跑 `bench_pipeline` 默认 8s 以外的 soak；可选一步
  `bench_pipeline --seconds 2` 作为冒烟，失败则红（进程崩溃才红，不要比 dropped）。

**Risks:**
- OpenCV 包体积导致超时：必须 cache。
- 装了 MinGW Qt → 与 FATAL_ERROR 冲突；必须 MSVC 套件。
- PATH 上 System32 旧 `onnxruntime.dll`：沿用现有「拷到构建目录」逻辑。
- `tst_cameramanager` / `tst_visioncontroller` 要插件 DLL：`POST_BUILD` copy 已存在，
  确认 ctest PATH 含 Qt/OpenCV/ORT（`tests/CMakeLists.txt` WIN32 段）。

**Required tests:**
- 本任务以 workflow 语法 + 本地用同一 cmake 命令复现为准。
- 若当时已能 push：看 Actions 日志；**不要**在文档里预先写「已全绿」除非看到绿勾。

**Acceptance criteria:**
- CPU job 不安装 CUDA。
- 无 GPU runner。
- README（T07）再挂 badge；本任务可先不改 README 以免两处冲突——**锁定 T07 加 badge**。

**Dependencies:** T01 labels，T05 preset。接口变更：否。并发风险：否。新第三方依赖：否（CI 下载已有依赖）。需测量：否。

**Recommended Git commit message:** `ci: add Windows CPU configure-build-ctest workflow`

---

### P9-T07  README 作品集化与上游归属

**Task ID:** P9-T07

**Task Name:** 重写 README / 更新 LICENSE 版权行

**Goal:** GitHub 首页描述 **VisionLab Pro 实际架构**，而不是上游摄像头 Demo 文案。
不写未测量的性能数字。

**Files likely affected:**
- `README.md`（整篇按 Phase 9 提示的标题结构重写，可用中英其一；**锁定中文正文 + 英文标题锚点**，
  或全英文。为跟现有 `docs/*.md` 英文契约一致：**README 用英文**，与 `docs/ui/qml-boundary.md` 一致。）
- `LICENSE`（保留 MIT 正文；Copyright 行增加 VisionLab Pro 作者，**不删除** Muhammed Suwaneh）
- `AGENTS.md`（把工作区已有的任务报告约定提交；若不想进本 commit 可留给 T10，
  **锁定本任务提交 AGENTS.md 报告段**，仍不要碰 `inference/` CRLF）

**New files:** 无（架构图文件是 T08）

**Interfaces affected:** 无代码。README 必含章节（可链到 docs）：

Overview，Features，Demo（可链 `screenshots/`，注明若截图仍是旧 UI 则「历史截图」），
Architecture（链 `docs/architecture.md`），Real-Time Pipeline，Inference，GPU，
Plugins，Tracking，Analytics，UI，Performance（链模板，表内 `[NOT MEASURED]`），
Project Structure，Build（链 `docs/build.md`），Configuration，Testing，
Benchmark（链 `docs/benchmarks.md`），Roadmap（Phase 9 完成后标 V1.0；未做项写 Future），
Upstream / Attribution。

Features 必须能在仓库里指到符号：`BoundedQueue` DropOldest、`IInferenceEngine`、
插件、ByteTrack、四规则、SQLite EventWriter、四页 QML。

明确 **不是**：C++17（实际 C++20）、检测器直接画框（已分离）、`std::thread` 主路径
（实际 `jthread`）。

Clone URL 改为 `https://github.com/NiHe-cell/VisionLab_Pro.git`，并写 upstream。

**Implementation outline:**
- 删除 `set(OpenCV_DIR "C:/opencv/build")` 这种易误导片段；改为 `docs/build.md`。
- Demo 节不贴假 FPS。
- T06 badge：`![CI](https://github.com/NiHe-cell/VisionLab_Pro/actions/workflows/ci.yml/badge.svg)`
  即使当时 workflow 还没绿，badge 也可以挂；**正文不要写 All tests passing 除非属实**。

**Risks:**
- 把上游截图说成当前四页监控台。
- Resume 式夸张（「生产级保证 60FPS」）。

**Required tests:** 无代码测试。人工：打开 README 每个内部链接文件存在。

**Acceptance criteria:**
- 读者能区分上游 Demo 与 Pro 二次开发。
- 无伪造基准。

**Dependencies:** T08 可稍后补链接；若 T08 未做，Architecture 节先放 ASCII 四行并注明图在 T08。
**锁定：T07 在 T08 之后执行**（顺序表已要求），README 直接链 Mermaid 文件。
接口变更：否。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `docs: rewrite README for VisionLab Pro and credit upstream`

---

### P9-T08  架构图

**Task ID:** P9-T08

**Task Name:** Mermaid 架构图

**Goal:** 五张与代码一致的图，供 README 与面试讲解。

**Files likely affected:** 无代码。

**New files:**
- `docs/architecture.md`

**Interfaces affected:** 无。

图（Mermaid `flowchart` / `sequenceDiagram`）：

1. Overall：QML → VisionController → CameraManager → PluginManager / VisionPipeline / EventWriter
2. Threading：GUI vs Capture `jthread` vs Inference `jthread` vs EventWriter `jthread`；
   队列 DropOldest；`QueuedConnection` 回 GUI
3. Inference：YoloDetector → IInferenceEngine → ORT CPU / ORT CUDA / TensorRT
4. Plugins：PluginManager → IVisionPlugin → IDetector；与 inference backend 不是同一扩展点
5. Detect → Track → Rules → Render → EventLog

每张图下用短句写 **与代码不符时以谁为准**（以源码为准）。不要画未实现的 RuleWorker。

**Implementation outline:**
- 节点名用真实类型名（`BoundedQueue<FramePacket>`、`LatestResult<PresentedFrame>`）。
- 不出现「保证实时 / 60FPS」标注。

**Risks:**
- 图画成热更新规则或 GUI 调 `evaluate`。
- 复制过期 Phase 7 空引擎为唯一生产形态（Phase 8 已可停机注入 RuleSpec）。

**Required tests:** 无。对照 `CameraManager.cpp` / `InferenceWorker.cpp` 人工审图。

**Acceptance criteria:**
- 五张图都反映 HEAD 行为（空引擎是默认，UI 可注入规则）。

**Dependencies:** 无（建议在 T07 前完成）。接口变更：否。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `docs: add architecture mermaid diagrams matching the pipeline`

---

### P9-T09  性能表模板、面试稿、简历占位

**Task ID:** P9-T09

**Task Name:** 可填写的测量模板 + 基于仓库的工程叙事

**Goal:** 面试材料只讲仓库里真实做了的取舍；数字全是占位或一次实跑粘贴。

**Files likely affected:** 无代码。

**New files:**
- `docs/performance.md`
- `docs/interview-notes.md`
- `docs/resume-evidence.md`

**Interfaces affected:** 无。

**`docs/performance.md`：**

表头：Backend | Precision | Model | Input | mean_ms | P50 | P95 | P99 | throughput_fps | GPU Memory | Notes

行：ORT CPU FP32、ORT CUDA FP32、TensorRT FP32、TensorRT FP16。单元格默认
`[NOT MEASURED]`。另附「如何填」：复制 `bench_onnx_cpu` stdout，并记录
OS / CPU / GPU / 驱动 / ORT 版本 / TensorRT 版本 / 构建类型 Debug|Release。
管线过载另表：`bench_pipeline` 的 dropped / peak_queue_depth，同样默认未测。
**禁止**把聊天里的旧数字抄进来。

**`docs/interview-notes.md`：** 十条对照 Phase 9 提示，每条必须能指到文件：

1. 有界队列 — `core/BoundedQueue.h`
2. DropOldest — `CaptureWorker` + `tst_pipelineoverload`
3. shutdown — `VisionPipeline::stop` / `jthread` / EventWriter 先停 pipeline
4. 推理抽象 — `IInferenceEngine` / factory
5. TensorRT — `docs/inference/tensorrt-engine.md` + cache 键
6. 插件 — `docs/plugins/detector-plugins.md`（V1 不卸载）
7. Detection vs Track — `IDetector` vs `ITracker`
8. 规则 — 脚点、转移事件、停机 mutate
9. 性能方法 — 250ms UI vs bench 进程；不把 UI 当基准
10. 真实踩坑 — 只写代码/文档里能证实的（例：System32 旧 ORT DLL；`PreserveAspectCrop`
    无法映回帧坐标；EventLog 与 `latest()->events` 丢帧）。没有的经历不要编。

**`docs/resume-evidence.md`：** 用括号占位：
`Reduced P95 from [BASELINE] ms to [FINAL] ms`。未测就保持占位。可写**可核实**的
非数字成就：四检测插件、四条规则、三条推理后端抽象、SQLite 10000 行 prune 等。

**Risks:**
- 把设计稿写成「我们测到了 3ms」。
- 把别人的 TensorRT 文章当本项目经验。

**Required tests:** 无。自检：三份文档搜索 `ms` / `FPS` / `60` 不得出现无标注来源的具体性能数。

**Acceptance criteria:**
- `[NOT MEASURED]` 或占位符存在；无假装已测。

**Dependencies:** T02/T03 CLI 键名（表列与 stdout 一致）。接口变更：否。并发风险：否。
新第三方依赖：否。需测量：否（模板阶段）。

**Recommended Git commit message:** `docs: add performance templates and interview notes without fake numbers`

---

### P9-T10  终态架构评审与 V1.0 清单

**Task ID:** P9-T10

**Task Name:** 问题分级 + 发布核对表

**Goal:** 对照真实代码列出剩余问题，给出 V1.0 是否可标的清单。不趁机大重构。

**Files likely affected:**
- `README.md` Roadmap 一句（V1.0 范围）
- `docs/testing.md` 若 T01 后有变更则补链

**New files:**
- `docs/architecture-review.md`
- `docs/release-checklist.md`

**Interfaces affected:** 无。

**`docs/architecture-review.md`：** 按 Blocker / High / Medium / Low / Future 列表。
每条：位置、现象、是否阻塞 V1.0。评审范围用 `docs/cursor/code-review.md` 的 30 项当清单，
但输出是「仓库现状」不是空模板。

已知可写入（实现时再核对是否仍属实，可改级别，**不要发明新的已修 bug**）：

- EventLog 256 vs 持久化窗口（`docs/storage/events.md`）
- 规则与 evaluate 非线程安全、无热更新
- 插件不卸载
- INT8 未做
- qmltestrunner 未做
- 默认构建无 sanitizer
- README 旧截图可能与四页 UI 不一致
- `models/` 目录名兼放 Qt Model 与权重文件（Future 改名，本阶段不改）
- 工作区 `inference/` 换行噪声（不要在本任务「顺手格式化」）

Blocker 仅当：默认 CPU 构建失败、GUI 跑推理、无界队列回潮、无测试的热路径数据竞争。
若无 Blocker，写明 **V1.0 可在清单勾完后标记**。

**`docs/release-checklist.md`：** 对照 Phase 9 DoD 逐条 checkbox：CPU 构建、ctest cpu、
ORT CPU、CUDA/TRT「环境支持时」、插件、跟踪、规则、QML 四页、SQLite、基准可复现、
文档属实、上游归属、无假数字。CUDA/TRT 在无 GPU 的机器上勾「N/A — not built」。

**Implementation outline:**
- 本任务不改生产 cpp。若评审发现 Blocker，**停下来单独开修复任务**，不要塞进 T10 文档 commit。
- 不运行小时 soak 却勾「long-running stable」。

**Risks:**
- 把 Future 写成 Blocker 导致永远不能 V1。
- 在评审 commit 里重构 CameraManager。

**Required tests:** 默认 `ctest -L cpu` 在文档中记录**本次实跑**通过/跳过数量（从 ctest 输出抄，
禁止编造）。若本环境未跑，清单写 `ctest: [NOT RUN THIS SESSION]`。

**Acceptance criteria:**
- 有分级债列表 + 发布清单。
- 无新功能代码。

**Dependencies:** T01–T09 文档已存在，避免评审漏文件。接口变更：否。并发风险：否。
新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `docs: add V1 architecture review and release checklist`

---

## 任务总览

| Task ID | 名称 | 依赖 | 接口变更 | 并发风险 | 新依赖 | 需测量 | 新测试 |
|---|---|---|---|---|---|---|---|
| P9-T01 | 测试清单 + labels | 无 | 否 | 否 | 否 | 否 | 否（预期） |
| P9-T02 | `bench_pipeline` | 无 | CLI | 是 | 否 | 否 | 是（CLI） |
| P9-T03 | `bench_tracker` | 无 | CLI | 否 | 否 | 否 | 是（CLI） |
| P9-T04 | soak 说明 | T02 | CLI 小 | 是 | 否 | 否 | 是（CLI） |
| P9-T05 | presets / 警告 option | 无 | CMake | 否 | 否 | 否 | 否 |
| P9-T06 | GitHub Actions CPU | T01, T05 | 否 | 否 | 否 | 否 | CI |
| P9-T07 | README + LICENSE | T08 | 否 | 否 | 否 | 否 | 否 |
| P9-T08 | Mermaid 图 | 无 | 否 | 否 | 否 | 否 | 否 |
| P9-T09 | 面试稿 / 性能表 | T02, T03 | 否 | 否 | 否 | 否 | 否 |
| P9-T10 | 评审 + 发布清单 | T01–T09 | 否 | 否 | 否 | 否 | 否 |

## Phase 9 DoD 对照

| DoD | 任务 |
|---|---|
| Clean CPU build | T05, T06 |
| Tests pass | T01 labels + 既有套件；T06 CI |
| Real-time pipeline stable | 既有 lifecycle 测试；T02/T04 工具与说明 |
| ONNX CPU | 既有；`bench_onnx_cpu` |
| CUDA / TensorRT where supported | 既有引擎；CI 不测；performance 表可 N/A |
| Plugins / tracking / rules / QML / SQLite | Phase 5–8 已完成；T10 勾选 |
| Benchmarks reproducible | T02, T03，既有 inference bench；T09 模板 |
| Docs describe actual behavior | T07–T10 |
| Upstream attribution | T07 LICENSE/README |
| No fake benchmark or resume claims | T09, T10；全局锁定第 1 条 |
| Interview notes | T09 |
| Final architecture review | T10 |
| qmltestrunner | **明确不做**，T10 Future |

指定 Task ID 之前不要写实现代码。

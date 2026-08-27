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

# Phase 6 — Multi-Object Tracking 任务分解

对照 HEAD `9e02281`（`feat(app): load detectors via PluginManager and drop DetectorFactory`）。
Phase 0–5 已收口。仓库里没有 `ITracker` / `Track` / `tracking/`。
`InferenceWorker` 在 detect 之后、render 之前留了 Tracking 插入点，且注明本阶段不增加线程。

工作区里 `inference/` 等文件有未提交 diff（增删行数相等，像是换行符噪声）。
Phase 6 提交不要把这些文件卷进去。

## 当前架构

```
QML (Main / CameraView)
  → CameraManager / VisionController
      → PluginManager → IVisionPlugin → unique_ptr<IDetector>
      → VisionPipeline
           CaptureWorker (std::jthread)
             → BoundedQueue<FramePacket>  DropOldest
           InferenceWorker (std::jthread)
             → IDetector::detect
             → 【Phase 6 插入 ITracker::update】
             → DetectionRenderer
             → LatestResult<PresentedFrame>
      → CameraImageProvider → QML
```

`PresentedFrame` 已有 `detections`，注释写明供 Tracking 插入。
QML 只吃 RGB 图；本阶段用叠加层显示 `Person #17`，不做 TrackModel（Phase 8）。
检测器插件、推理后端、规则引擎本阶段不动。

## 锁定的设计决策

1. **插入位置**：跟在 InferenceWorker 里，detect 之后、render 之前。不新增 jthread，不加第二段有界队列。采集已经靠 DropOldest 解耦；ByteTrack 是框关联，不读像素，不必单独占一个流水线阶段。
2. **实现**：自研 BYTE 关联（Zhang et al., ECCV 2022），不 vendor 任何 MOT 库，不加 Eigen / 新第三方依赖。Kalman 用已有的 `cv::KalmanFilter`。
3. **匹配**：V1 用**同类 greedy IoU**（同一 `classId` 才匹配）。不做 Hungarian。
4. **ITracker 不接收 `cv::Mat`**：`update(detections, TrackUpdateContext{frameId, timestamp})`。跟踪器不碰像素。
5. **检测器不知道跟踪器**。不改 `IDetector`、不改插件 IID / capabilities、不把 tracking 做成插件。
6. **`update()` 返回** Confirmed 与仍存活的 Lost（含预测框）。不返回 Tentative、不返回 Removed。
7. **轨迹**：`std::deque<TrackPoint>`，默认最多 30 个质心；超限 `pop_front`。禁止无限 vector。
8. **`trackId`**：`std::uint64_t`，会话内单调递增、不复用；`reset()` 后从 1 再分配。
9. **模式切换**：只在推理线程、下一次 `update` 前 `reset()`。GUI 的 `setMode` 不得直接调 `ITracker::reset()`。
10. **空 tracker**：`ITracker*` 为空时行为与现在相同（只画 Detection）。生产 `CameraManager` 注入 `ByteTrackTracker`。测试默认可传空。
11. **叠加**：有 tracks 时画 `label #id` 和轨迹折线；tracks 为空则回退 `DetectionRenderer`（`minHits` 未确认前仍能看到框）。
12. **本阶段不做**：QML TrackModel / 设置页开关、独立 TrackingWorker、INT8、规则引擎、伪造 FPS。延迟只进 `PipelineStats`，不写 benchmark 可执行文件（那是 Phase 9）。

默认 `TrackerConfig`：`highThresh=0.6`，`lowThresh=0.1`，`matchIou=0.5`，`maxLostFrames=30`，`minHits=3`，`maxTrajectoryPoints=30`。

放弃的备选：独立 TrackingWorker（多一段队列和关机路径，跟踪本身很便宜）；vendor ByteTrack C++（体积与许可证不可控）；用插件 id 扩展跟踪（规格禁止把跟踪放进检测插件）；Hungarian（V1 目标数很小，greedy 可测、可换）。

## 推荐执行顺序

`P6-T01 → T02 → T03 → T04 → T05 → T06 → T07 → T08`

T03 与 T05 在 T02 之后理论上可并行；T05 / T06 / T07 都会改 `InferenceWorker`，单会话必须串行。T08 必须等 T04（真正的 ByteTrack）和 T06/T07（叠加与指标）都绿。

---

### P6-T01  Track 领域类型

**Task ID:** P6-T01

**Task Name:** Track 领域类型

**Goal:** 先有可编译、可断言的 `Track` / 生命周期枚举 / 配置与统计结构。本任务不写跟踪算法、不接管线。

**Files likely affected:**
- `CMakeLists.txt`（`visionlab_core` 加入 `core/Track.h`）
- `tests/CMakeLists.txt`（`tst_track`，Windows PATH 列表加上 `track`）

**New files:**
- `core/Track.h`
- `tests/TrackTest.cpp`

**Interfaces affected:** 新增（均在 `visionlab` 命名空间）。不改 `Detection`。

```cpp
enum class TrackState : std::uint8_t { Tentative, Confirmed, Lost };

struct TrackPoint {
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
    cv::Point2f centroid{};
    cv::Rect box;
};

struct Track {
    std::uint64_t trackId = 0;
    int classId = -1;
    std::string label;
    float confidence = 0.0F;
    cv::Rect box;
    TrackState state = TrackState::Tentative;
    std::chrono::steady_clock::time_point firstSeen{};
    std::chrono::steady_clock::time_point lastSeen{};
    int age = 0;              // 自创建起经过的帧数
    int hits = 0;             // 成功匹配帧数
    int timeSinceUpdate = 0;  // 连续未匹配帧数
    std::deque<TrackPoint> trajectory;
};

struct TrackerConfig {
    float highThresh = 0.6F;
    float lowThresh = 0.1F;
    float matchIou = 0.5F;
    int maxLostFrames = 30;
    int minHits = 3;
    std::size_t maxTrajectoryPoints = 30;
};

struct TrackerStats {
    std::size_t activeTracks = 0;
    std::uint64_t createdTracks = 0;
    std::uint64_t lostTracks = 0;
    std::uint64_t removedTracks = 0;
    double lastUpdateLatencyMs = 0.0;
};

struct TrackUpdateContext {
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
};
```

`Track.h` 纯领域，不依赖 Qt。`core` 仍不链接 Qt。

**Implementation outline:**
- 头文件放 `core/`，与 `Detection.h` 同层。
- `trajectory` 用 `deque`，本任务只定类型，不写裁剪函数。
- 不改 `PresentedFrame`（T05 再加 `tracks`）。
- 不引入 `ITracker`。

**Risks:**
- 把 `cv::Mat` 放进 `Track` 会让 LatestResult 快照变重，且跟踪器不需要像素。
- `trackId` 用 `int` 长时间运行会溢出；必须 `uint64_t`。
- 无限 `vector` 轨迹会在 Phase 7 长跑时撑爆内存——类型层就用 deque + 容量字段。

**Required tests:**
- 默认 `Track`：`trackId==0`，`state==Tentative`，`trajectory.empty()`。
- 默认 `TrackerConfig` 数值与上文锁定值逐项相等。
- `TrackUpdateContext` 默认 `frameId==0`。
- `visionlab_core` 目标仍不链接 Qt。

**Acceptance criteria:**
- 后续任务只 `#include "core/Track.h"`，不再发明另一套 Track 字段名。
- 生产管线零行为变化。

**Dependencies:** 无。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(core): add Track domain types for multi-object tracking`

---

### P6-T02  ITracker 契约 + FakeTracker + 算法文档

**Task ID:** P6-T02

**Task Name:** ITracker 契约与 BYTE 说明

**Goal:** 把跟踪扩展点写成可编译契约，并用短文档锁死 BYTE 两段匹配、生命周期和管线插入点。本任务不实现 ByteTrack，不改 InferenceWorker。

**Files likely affected:**
- `CMakeLists.txt`（新增 `visionlab_tracking`：本任务可以是仅头文件静态库，`LINKER_LANGUAGE CXX`；链 `visionlab_core`，不链 Qt）
- `tests/CMakeLists.txt`（`tst_itracker`）

**New files:**
- `tracking/ITracker.h`
- `tests/fakes/FakeTracker.h`
- `tests/ITrackerTest.cpp`
- `docs/tracking/bytetrack.md`

**Interfaces affected:**

```cpp
class ITracker {
public:
    virtual ~ITracker() = default;
    virtual std::string name() const = 0;
    virtual std::vector<Track> update(
        const std::vector<Detection>& detections,
        const TrackUpdateContext& context) = 0;
    virtual void reset() = 0;
    virtual TrackerStats stats() const = 0;
    virtual TrackerConfig config() const = 0;
};
```

线程约定（写进头注释与文档）：同一实例的 `update`/`reset` 不承诺可并发；只由推理线程调用。`update` 不得修改入参 Detection，不得访问像素。

`FakeTracker`（仅测试）：`name()=="fake"`；对非空 detections 按输入顺序生成 Confirmed track，`trackId` 从 1 起对应当前帧下标（**不跨帧保 ID**）；空输入返回 `{}`；`reset` 把内部计数清零；`stats().createdTracks` 累加本次产出条数。用来测管线接线，不冒充 BYTE。

**Implementation outline:**
- `docs/tracking/bytetrack.md` 必须写清：高分框 / 低分框切分；先 `predict` 再匹配；第一段高分↔轨迹；未匹配高分→新建 Tentative；第二段低分↔未匹配轨迹（遮挡恢复）；低于 `lowThresh` 丢弃；未匹配轨迹 `timeSinceUpdate++`，超过 `maxLostFrames` → Removed；`minHits` 才升 Confirmed；同类 IoU greedy；Kalman 状态为 SORT 风格 xyah（cx, cy, aspect, height）及速度；`dt` 来自相邻 `timestamp`，首帧 `dt=1/30`；独立实现，不是拷贝 yolox/byte_tracker；原论文 MIT/算法描述，本仓库不引入第三方 MOT 源码。
- 文档写明插入点：InferenceWorker detect 之后、render 之前；以及为什么不单独开 Tracking 线程。
- 能力列表不要预留 plugin / QML 字段。
- 不改检测插件、不改 `IDetector`。

**Risks:**
- `update` 若吃 `FramePacket` 会暗示跟踪器可以写 `image`，破坏只读契约。
- 文档若留「以后再决定 greedy/Hungarian」，T04 会分叉。
- FakeTracker 若做跨帧 ID，T05 会误以为 BYTE 已实现。

**Required tests:**
- FakeTracker：两条 Detection → 两条 Confirmed track，id 为 1 和 2，box/label/classId/confidence 拷自 Detection。
- 空 detections → 空 tracks。
- `reset` 后下一帧仍从 1 编号。
- 头文件可被测试 include，且 `visionlab_tracking` 不链接 Qt。

**Acceptance criteria:**
- T04 只实现文档里的 BYTE，不再改 `ITracker` 签名。
- 生产路径零行为变化。

**Dependencies:** T01。接口变更：是。并发风险：否（只定契约）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(tracking): add ITracker contract and BYTE design notes`

---

### P6-T03  IoU greedy 匹配 + Kalman 框滤波

**Task ID:** P6-T03

**Task Name:** 匹配与预测原语

**Goal:** 把 BYTE 依赖的几何与滤波做成可单独测的函数/类型。本任务不实现轨迹生命周期，不接管线。

**Files likely affected:**
- `CMakeLists.txt`（`visionlab_tracking` 加入源文件）
- `tests/CMakeLists.txt`（`tst_ioumatching`、`tst_kalmanboxfilter`）

**New files:**
- `tracking/IouMatching.h`
- `tracking/IouMatching.cpp`（若头文件无法 inline 干净；允许纯头文件）
- `tracking/KalmanBoxFilter.h`
- `tracking/KalmanBoxFilter.cpp`
- `tests/IouMatchingTest.cpp`
- `tests/KalmanBoxFilterTest.cpp`

**Interfaces affected:**

```cpp
float intersectionOverUnion(const cv::Rect& a, const cv::Rect& b); // 并集为 0 时返回 0

struct Association {
    int detectionIndex = -1;
    int trackIndex = -1;
    float iou = 0.0F;
};

// 只在 classIds[i]==trackClassIds[j] 且 IoU>=threshold 时匹配。
// greedy：反复取剩余对中 IoU 最大者。每个 detection/track 至多匹配一次。
std::vector<Association> greedyIouAssociate(
    const std::vector<cv::Rect>& detectionBoxes,
    const std::vector<int>& detectionClassIds,
    const std::vector<cv::Rect>& predictedTrackBoxes,
    const std::vector<int>& trackClassIds,
    float iouThreshold);

class KalmanBoxFilter {
public:
    explicit KalmanBoxFilter(const cv::Rect& initialBox);
    cv::Rect predict(double dtSeconds);
    cv::Rect update(const cv::Rect& measurement);
    cv::Rect box() const;
};
```

Kalman：8 维状态 `[cx,cy,a,h,vx,vy,va,vh]`，测量 4 维 `[cx,cy,a,h]`。`a = w/max(h,eps)`。`predict` 在 `dt<=0` 时按 `1/30` 处理。只用 OpenCV，不引入新库。

**Implementation outline:**
- IoU 对空框 / 无重叠返回 0，不得 NaN。
- 不同 `classId` 即使框完全重合也不进 `Association`。
- Kalman 实现放 `tracking/`，检测器不得 include。
- 不在本任务写 BYTE 状态机。

**Risks:**
- `cv::Rect` 整数框与浮点 cx,cy 往返会抖 1px；测试用 IoU 或中心距离容差，不要像素级咬死 predict 后的 Rect。
- `dt` 单位搞错（毫秒当秒）会让速度爆炸。
- 在匹配里做跨类关联会导致人/车抢 ID。

**Required tests:**
- 完全重合 IoU==1；不相交 IoU==0；一半重叠约 1/3（用已知矩形算精确值）。
- 两对高 IoU、一对低 IoU：greedy 只留下两条，且不交叉占用。
- 相同位置、不同 classId：关联为空。
- Kalman：静止框多次 predict+同一测量 update，中心仍在初始附近（容差数像素）。
- 向右平移的测量序列，predict 的 cx 单调不减。

**Acceptance criteria:**
- T04 只组合这些原语，不再手写 IoU。
- 不改检测 / 插件 / 管线。

**Dependencies:** T01。接口变更：是（tracking 内部 API）。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(tracking): add greedy IoU association and Kalman box filter`

---

### P6-T04  ByteTrackTracker

**Task ID:** P6-T04

**Task Name:** ByteTrackTracker（BYTE 生命周期）

**Goal:** 唯一生产 `ITracker` 实现。用合成序列锁住 ID 稳定、漏检恢复、过期、reset、轨迹有界。不接管线、不画框。

**Files likely affected:**
- `CMakeLists.txt`
- `tests/CMakeLists.txt`（`tst_bytetracktracker`）

**New files:**
- `tracking/ByteTrackTracker.h`
- `tracking/ByteTrackTracker.cpp`
- `tests/ByteTrackTrackerTest.cpp`

**Interfaces affected:**

```cpp
class ByteTrackTracker final : public ITracker {
public:
    explicit ByteTrackTracker(TrackerConfig config = {});
    std::string name() const override; // "ByteTrack"
    std::vector<Track> update(const std::vector<Detection>&, const TrackUpdateContext&) override;
    void reset() override;
    TrackerStats stats() const override;
    TrackerConfig config() const override;
};
```

算法（必须与 `docs/tracking/bytetrack.md` 一致）：
1. 用上一帧时间戳算 `dt`，对所有未移除轨迹 `predict`。
2. 检测按 conf 分成高（`> highThresh`）与低（`lowThresh < conf <= highThresh`）。
3. 高分 ↔ 非 Removed 轨迹，`greedyIouAssociate`。
4. 未匹配高分 → 新 Tentative，`createdTracks++`，分配下一个 `trackId`。
5. 低分 ↔ 仍未匹配轨迹（第二段）。
6. 仍未匹配的轨迹：`timeSinceUpdate++`，`state=Lost`（首次进入 Lost 时 `lostTracks++`）；`timeSinceUpdate > maxLostFrames` → Removed（`removedTracks++`，不再返回）。
7. 匹配成功：`update` Kalman，`hits++`，`age++`，`timeSinceUpdate=0`；`hits>=minHits` → Confirmed。
8. 每条仍存活轨迹把当前质心推进 `trajectory`；`size>maxTrajectoryPoints` 则 `pop_front`。
9. 返回 Confirmed + Lost。Tentative / Removed 不出现在返回值。
10. `activeTracks` = 返回值条数。`lastUpdateLatencyMs` 为这次 `update` 墙钟。
11. `reset()`：清空轨迹，id 生成器回到 1，计数器清零（含 stats）。

**Implementation outline:**
- 内部可有 `InternalTrack`（Kalman + 状态），对外只暴露 `Track`。
- 测试构造 `TrackerConfig{ .minHits = 1, .maxLostFrames = 3, .maxTrajectoryPoints = 5 }` 以便短序列。
- 合成 Detection 只改 `box` / `confidence` / `classId` / `label`，不读真实视频。
- 不链接 Qt。不改插件。

**Risks:**
- `minHits=3` 的测试若只喂 2 帧会误判「没建轨」。
- 漏检恢复必须走低分第二段；只测高分会漏掉 BYTE 的卖点。
- `reset` 后仍复用旧 Kalman / 旧 id 会污染模式切换（T05/T08）。
- 轨迹不裁剪会让测试过、长跑失败。

**Required tests:**
- 同一框连续 10 帧（minHits=1）：始终同一个 `trackId`。
- 中间空 2 帧再在附近出现（maxLostFrames=5）：仍同一 ID。
- 空闲超过 `maxLostFrames` 再出现：新 ID，且 `removedTracks>=1`。
- 同时两个分开的框：两个 ID；对调位置但 IoU 仍能一对一（相向远离，不交叉穿过）。
- 不同 classId、框重合：两条轨迹，不合并。
- 高分建轨后消失，下一帧只给低分近框：收回同一 ID。
- 连续空 detections：Lost 一段时间后返回空。
- `reset` 后新框的 `trackId==1`。
- 单轨跑 40 帧：`trajectory.size()==maxTrajectoryPoints`。
- 默认构造 `name()=="ByteTrack"`。

**Acceptance criteria:**
- 检测器源码零修改。
- 无第三方 MOT 源码进入仓库。
- 内存策略有测试锁住。

**Dependencies:** T02, T03。接口变更：是。并发风险：否（单线程单元测试）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(tracking): implement ByteTrackTracker with bounded trajectories`

---

### P6-T05  管线插入：PresentedFrame.tracks + InferenceWorker

**Task ID:** P6-T05

**Task Name:** 在 InferenceWorker 接入 ITracker

**Goal:** 推理线程在 detect 之后调用 `update`，结果进入 `PresentedFrame.tracks`。模式变化时在推理线程 `reset`。本任务仍用 DetectionRenderer 画框（T06 再换 Track 叠加）。生产 CameraManager 本任务仍可不注入 ByteTrack（T08 注入）。

**Files likely affected:**
- `core/PresentedFrame.h`（增加 `std::vector<Track> tracks`）
- `pipeline/InferenceWorker.h` / `.cpp`
- `pipeline/VisionPipeline.h` / `.cpp`（可选 `unique_ptr<ITracker>`，默认空）
- `tests/InferenceWorkerTest.cpp`
- `tests/VisionPipelineTest.cpp`
- `CMakeLists.txt`（`visionlab_pipeline` 链接 `visionlab_tracking`）

**New files:** 无（FakeTracker 已在 T02）

**Interfaces affected:**
- `PresentedFrame` 增加 `tracks`，默认空。`detections` 保留。
- `InferenceWorker` 构造增加 `ITracker* tracker = nullptr`（或等价默认），放在现有默认参数之后，保证旧测试不用改调用。
- 增加 `ModeProvider = std::function<DetectionMode()>`，默认空；非空且 tracker 非空时，若本次 mode 与上次不同则先 `tracker->reset()` 再 `update`。
- `VisionPipeline` 构造增加 `std::unique_ptr<ITracker> tracker = {}`。`start()` 里把 `m_tracker.get()` 传给 worker，并把 `[this] { return mode(); }` 作 ModeProvider。成员声明：tracker 活得比 worker 长（先声明 tracker，`joinWorkers` 仍先 `reset` worker）。
- `ITracker` 为空：不调用 `update`，`tracks` 保持空，现有检测叠加不变。
- `update` 包在 try/catch（与 detect 相同策略）：异常则 `tracks.clear()`，打日志，不得让 worker 退出。
- 禁止在 `VisionPipeline::setMode`（GUI 可调用）里直接 `m_tracker->reset()`。

**Implementation outline:**
- 预留点注释改成实际调用：`context.frameId = packet.frameId`，`context.timestamp = packet.captureTimestamp`。
- 本任务不改 `DetectionRenderer`。
- 本任务不改 `PipelineStats`（T07）。
- `CameraManager` 本任务不 new ByteTrackTracker。
- `LatestResult<PresentedFrame>` 仍靠值拷贝；`Track` 无 Mat，额外开销可接受。

**Risks:**
- GUI `setMode` 与推理 `update` 并发：tracker 内部无锁。必须把 reset 放在推理线程。
- Worker 持有 `ITracker&` 而 pipeline 先毁 tracker：UAF。所有权只在 pipeline，worker 只持裸指针，析构顺序要测 start/stop。
- 把 `unique_ptr<ITracker>` 默认参数插在中间会弄断现有调用点。
- 模式切换不 reset 会让人脸 Kalman 去跟 YOLO 的车。

**Required tests:**
- tracker 为空：`presented.tracks.empty()`，detections 与现在一致（沿用 publishesDetectionsAndConvertsToRgb）。
- FakeTracker：detections 非空时 `tracks.size()==detections.size()`，`tracks[0].label` 与 detection 一致。
- 空 detections：tracks 空（FakeTracker 契约）。
- VisionPipeline + FakeTracker：`latest()->tracks` 非空（FakeDetector 固定一条）。
- 切换 mode：推理若干帧后 `setMode`，FakeTracker 的 id 重新从 1 开始（在 FakeTracker 上可观察 reset 被调用：给 FakeTracker 加 `resetCount()`，仅测试头）。
- 现有 InferenceWorker / VisionPipeline / overload / lifecycle 测试仍绿。

**Acceptance criteria:**
- 采集线程仍不跑跟踪。
- GUI 线程仍不跑 `update`。
- 检测器仍不 include `ITracker`。
- 无 ByteTrack 时行为与 Phase 5 相同。

**Dependencies:** T02（可在 T04 之前，用 FakeTracker）。接口变更：是。并发风险：是。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(pipeline): run ITracker after detect before render`

---

### P6-T06  Track 叠加：ID 与有界轨迹

**Task ID:** P6-T06

**Task Name:** TrackRenderer

**Goal:** UI 能在画面上看到 `Person #17` 以及可选轨迹。QML 不直接碰 tracker。不改 DetectionRenderer 的 Detection 语义，现有 `tst_detectionrenderer` 保持绿。

**Files likely affected:**
- `CMakeLists.txt`（`visionlab_rendering` 加入 TrackRenderer）
- `pipeline/InferenceWorker.cpp`（有 tracks 时走 TrackRenderer，否则 DetectionRenderer）
- `tests/CMakeLists.txt`（`tst_trackrenderer`）
- `tests/InferenceWorkerTest.cpp`（断言 RGB 上能看到标注变化或标签格式；能测则测，不能像素咬死则测 tracks 非空且走新路径的集成断言）

**New files:**
- `rendering/TrackRenderer.h`
- `rendering/TrackRenderer.cpp`
- `tests/TrackRendererTest.cpp`

**Interfaces affected:**

```cpp
class TrackRenderer {
public:
    void render(cv::Mat& frame, const std::vector<Track>& tracks) const;
};
```

- 标签格式：`label + " #" + std::to_string(trackId)`，例如 `person #17`（label 原样，不在渲染层做中英翻译）。
- 画 `track.box`；颜色规则与 DetectionRenderer 相同（`classId==kMotionClassId` 黄，否则绿）。
- `trajectory.size()>=2` 时按质心画折线，点坐标四舍五入到像素；轨迹点在画面外则跳过该段，不得崩溃。
- 空 tracks / 空帧：无操作。
- InferenceWorker：`!presented.tracks.empty()` 则 `TrackRenderer`，否则保持 `DetectionRenderer`（Tentative 未出结果时仍显示检测框）。

**Implementation outline:**
- 不要让 QML 解析 Track。
- 不要在检测插件里画 ID。
- 线宽/字体与 DetectionRenderer 现有常量对齐，避免两种叠加风格。
- 不在本任务改 Stats。

**Risks:**
- 在 DetectionRenderer 上加 Track 重载会让 Detection 测试耦合 ID 格式。
- 轨迹点用 float 直接当 int 可能越界写像素——必须夹到图像范围或依赖 OpenCV 裁剪并测越界不崩。
- 无 tracks 时不回退会导致 `minHits=3` 前画面没有框。

**Required tests:**
- 空列表 / 空帧：与 DetectionRenderer 相同的 no-op 断言。
- 一条 Confirmed `label="person"`, `trackId=17`, box `(10,10,20,20)`：`(10,10)` 为绿色；不要断言整串文字像素，但可 `putText` 后非零像素增加。
- 两条轨迹点：折线经过的像素相对纯框有额外非零（或至少 render 不崩、尺寸不变）。
- 越界 box / 负坐标：尺寸不变、不抛异常。
- InferenceWorker + FakeTracker：呈现帧 `tracks` 非空且仍输出连续 RGB。
- `tst_detectionrenderer` 全绿。

**Acceptance criteria:**
- 打开相机跑 Object/Face 时（T08 注入后）画面标签含 `#` 与稳定数字。本任务用 FakeTracker 证明格式。
- QML 文件本任务零修改。

**Dependencies:** T01, T05。接口变更：是。并发风险：否（仍在推理线程画）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(rendering): overlay track IDs and bounded trajectories`

---

### P6-T07  跟踪指标进入 PipelineStats

**Task ID:** P6-T07

**Task Name:** 跟踪延迟与轨迹计数

**Goal:** `PipelineStats` 能反映活跃轨迹数、创建/丢失/移除累计，以及最近一次（或窗口平均）跟踪耗时。不造 FPS 数字。不做独立 benchmark 可执行文件。

**Files likely affected:**
- `core/PipelineStats.h`
- `pipeline/StatsProbe.h` / `.cpp`
- `pipeline/InferenceWorker.cpp`（`update` 前后计时，调用 `onTracked`）
- `tests/StatsProbeTest.cpp`
- `tests/InferenceWorkerTest.cpp`（可选：FakeTracker 路径 `activeTracks>=1`）

**New files:** 无

**Interfaces affected:**
`PipelineStats` 增加：

```cpp
std::size_t activeTracks = 0;
std::uint64_t createdTracks = 0;
std::uint64_t lostTracks = 0;
std::uint64_t removedTracks = 0;
double avgTrackingLatencyMs = 0.0;
```

`StatsProbe::onTracked(const TrackerStats& stats, double trackingLatencyMs)`：在已有互斥下写入计数，并用现有或单独的 `LatencyWindow` 记跟踪耗时（单独 window，避免和推理延迟混在一个百分位里）。`snapshot()` 填 `avgTrackingLatencyMs`。`reset()` 清零新字段。

无 tracker 时 worker 不调用 `onTracked`，新字段保持 0。

不增加 `trackingFps`（与 `inferenceFps` 同线程，无新信息）。

**Implementation outline:**
- `onInferred` 语义不变：仍只记录检测耗时。跟踪耗时不计入 `avgInferenceLatencyMs`。
- QML 本阶段不展示这些字段（Phase 8 Performance 页）。测试断言即可。
- 禁止在测试里写死「必须 < X ms」。

**Risks:**
- 把跟踪耗时加进推理 latency 会让 Phase 3/4 基线不可比。
- `onTracked` 与 `onInferred` 锁顺序要保持「同一把 `m_mutex`」，避免新锁。
- `TrackerStats` 是累计值还是瞬时值必须与 T04 一致：created/lost/removed 累计，active 瞬时。

**Required tests:**
- 空探针：新字段全 0。
- `onTracked` 两次：`createdTracks` 取最后一次传入的累计值（探针覆盖写入累计，不自己 ++）。
- `avgTrackingLatencyMs` 对 10 和 30 的两次记录为 20。
- `reset` 后新字段为 0。
- `tst_statsprobe` 旧用例仍绿。
- InferenceWorker + FakeTracker：`stats().activeTracks` 在有检测时 > 0（若 FakeTracker.stats 填了 activeTracks）。

**Acceptance criteria:**
- 指标可从 `VisionPipeline::stats()` 读到。
- 文档/测试不出现编造的 FPS。

**Dependencies:** T05（T04 的真实累计在 T08 才进生产）。接口变更：是。并发风险：是（StatsProbe 已有锁，worker 与 UI snapshot）。新第三方依赖：否。需测量：否（只记录，不设门槛）。

**Recommended Git commit message:** `feat(pipeline): record tracker counts and tracking latency`

---

### P6-T08  生产接线：CameraManager 注入 ByteTrackTracker

**Task ID:** P6-T08

**Task Name:** 生产路径启用 ByteTrack

**Goal:** 真实管线拥有 `ByteTrackTracker`。start/stop 后 ID 从 1 再分配。写清跟踪与插件/推理的边界。QML 仍无设置页。

**Files likely affected:**
- `utilities/CameraManager.cpp`（组装 `VisionPipeline` 时 `make_unique<ByteTrackTracker>()`）
- `utilities/CameraManager.h`（注释：pipeline 拥有 tracker）
- `pipeline/VisionPipeline.cpp`（`start()` 在启动 jthread 前 `m_tracker->reset()`，保证重复 start 不沿用旧 ID；仍不要在 GUI `setMode` 里 reset）
- `docs/tracking/bytetrack.md`（补：生产默认开启、模式切换在推理线程 reset、与 IDetector 插件无关、V1 无热更新配置）
- `tests/CameraManagerTest.cpp`（Fake source 路径若可注入 pipeline：用带 ByteTrack 的 VisionPipeline 跑若干帧，`latest()->tracks` 在 Object 模式下非空或至少不崩溃；不要要求 GPU）
- `tests/PipelineLifecycleTest.cpp`（start/stop/start 后 tracker 仍安全；若 pipeline 测试可传入 ByteTrackTracker）
- `CMakeLists.txt`（app 已链 pipeline；确认 tracking 经 pipeline 链入，app **不要** include ByteTrack 头也可以——允许只在 CameraManager.cpp include）

**New files:** 无

**Interfaces affected:**
- 生产构造：`VisionPipeline(source, detectors, queueCapacity, make_unique<ByteTrackTracker>())`。若当前构造顺序不方便，用默认参数或增加只多一个 tracker 的重载，避免破坏测试里三参数构造。
- 不增加 QML property。
- 不增加 `VISIONLAB_TRACKING` 环境变量（Phase 8 设置页再做开关）。V1 生产始终跟踪。
- Motion 模式也走同一 tracker（运动块 ID 可能不稳，文档写为已知限制）。

**Implementation outline:**
- `CameraManager` 继续先建 PluginManager 再 pipeline；tracker 在 pipeline 内，不由插件创建。
- Grep：`detectors/` 与 `plugins/` 不得出现 `ITracker` / `ByteTrack`。
- 重复 `start/stop`：`joinWorkers` 后下次 `start` `reset` tracker。
- 更新 `InferenceWorker` 顶部注释，去掉「本阶段不增加线程」的过期「预留」说法，改为「跟踪在本线程、detect 与 render 之间」。

**Risks:**
- `minHits=3` 时前几帧 `tracks` 空，画面靠 DetectionRenderer 回退，测试不要断言第一帧就有 trackId。
- CameraManager 测试若仍注入不带 tracker 的 VisionPipeline，生产路径覆盖不到——至少一条接近生产装配的测试要带 ByteTrackTracker。
- 把 ByteTrack 链进 `visionlab_plugin` 会污染插件层。
- 工作区里未提交的 inference 换行改动不要 `git add`。

**Required tests:**
- VisionPipeline + ByteTrackTracker + FakeDetector（minHits 对 Fake 的 0.9 conf 走高分）：多帧后 `tracks.size()==1` 且 `trackId` 稳定。
- `stop` 再 `start`：新会话第一条确认轨迹 `trackId==1`。
- `setMode` Face→Object：推理线程 reset，Object 侧新 ID 从 1 起（可用 FakeDetector 双模式）。
- 全量 ctest：默认 CPU 构建，无 GPU 项不得无故变红。
- 插件测试与 YOLO/Face/Motion 单元测试仍绿。

**Acceptance criteria（对照 Phase 6 DoD）：**
- 持久 track ID 存在。
- 跟踪器实现 `ITracker`。
- 检测器不知道 tracker。
- 生命周期有定义（文档 + T04/T08 测试）。
- 轨迹内存有界（T04）。
- UI 能显示 ID（T06 叠加，经本任务进入生产画面）。
- 管线仍响应（不新增阻塞 GUI 的调用）。
- 跟踪测试通过。

**Dependencies:** T04, T06, T07。接口变更：是（生产默认行为：画面出现 `#id`）。并发风险：是（start/stop/reset）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(app): enable ByteTrack on the production vision pipeline`

---

## 任务总览

| Task ID | 名称 | 依赖 | 接口变更 | 并发风险 | 新依赖 | 需测量 | 新测试 |
|---|---|---|---|---|---|---|---|
| P6-T01 | Track 领域类型 | 无 | 是 | 否 | 否 | 否 | 是 |
| P6-T02 | ITracker + BYTE 文档 | T01 | 是 | 否 | 否 | 否 | 是 |
| P6-T03 | IoU + Kalman 原语 | T01 | 是 | 否 | 否 | 否 | 是 |
| P6-T04 | ByteTrackTracker | T02, T03 | 是 | 否 | 否 | 否 | 是 |
| P6-T05 | InferenceWorker 接入 | T02 | 是 | 是 | 否 | 否 | 是 |
| P6-T06 | TrackRenderer | T01, T05 | 是 | 否 | 否 | 否 | 是 |
| P6-T07 | PipelineStats 跟踪字段 | T05 | 是 | 是 | 否 | 否 | 是 |
| P6-T08 | 生产注入 ByteTrack | T04, T06, T07 | 是 | 是 | 否 | 否 | 是 |

## Phase 6 DoD 对照

| DoD | 任务 |
|---|---|
| Persistent track IDs | T04, T08 |
| ITracker | T02, T04 |
| Detector unaware of tracker | T05–T08 Grep |
| Track lifecycle defined | T02 文档, T04 |
| Memory does not grow indefinitely | T04 trajectory 测试 |
| UI can display IDs | T06, T08 |
| Pipeline remains responsive | T05 不新增 GUI 阻塞；不新开线程 |
| Tracking tests pass | T04 合成序列 + T05/T08 管线 |

指定 Task ID 之前不要写实现代码。

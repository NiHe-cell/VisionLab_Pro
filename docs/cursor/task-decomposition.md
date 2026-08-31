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

# Phase 7 — Intelligent Event / Rule Engine 任务分解

对照 HEAD `cec53c1`（`feat(app): enable ByteTrack on the production vision pipeline`）。
Phase 0–6 已收口。仓库里没有 `IRule` / `VisionEvent` / `analytics/`。
`InferenceWorker` 在 `ITracker::update` 之后、render 之前可以插入规则求值，且本阶段不增加线程。

工作区里 `inference/` 等文件有未提交换行符噪声，`AGENTS.md` 有任务报告约定的未提交增补。
Phase 7 提交不要把这些文件卷进去。

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
             → ITracker::update
             → 【Phase 7 插入 RuleEngine::evaluate】
             → TrackRenderer / DetectionRenderer
             → LatestResult<PresentedFrame>
      → CameraImageProvider → QML
```

`PresentedFrame` 已有 `detections` 与 `tracks`。QML 仍只吃 RGB 图。
本阶段不画 ROI / 越线，不做 EventModel / SQLite / 设置页（Phase 8）。
检测器插件、推理后端、ByteTrack 算法本阶段不动。

## 锁定的设计决策

1. **插入位置**：跟在 InferenceWorker 里，track 之后、render 之前。不新增 jthread，不加第二段帧队列。规则只读 Track（框 + id + 时间），不读像素，不必单独占一个流水线阶段。
2. **库边界**：新建 `visionlab_analytics`，链 `visionlab_core`，不链 Qt。规则不是插件，不进 `IVisionPlugin` capabilities，不改检测器 / ByteTrackTracker。
3. **Inside 判定**：目标是否在 ROI 内，用包围框**底边中点**（foot point）：`(box.x + width/2, box.y + height)`。含边界（`cv::pointPolygonTest >= 0` 视为 inside）。不采用中心点或重叠比——人/车贴地，中心点会在脚还在线外时提前触发。
4. **矩形 ROI**：配置只存多边形（`vector<cv::Point2f>`，至少 3 点）。提供 `polygonFromRect(cv::Rect)` 把矩形变成四顶点，测试用矩形即可。
5. **越线方向**：有向线段 A→B。点在 AB 左侧（叉积 > 0）为 +，右侧为 −。完整穿越 = 上一帧与本帧异号且都不为 0，且 `prev→curr` 与 `A→B` 相交。
   - `CrossingDirection::Forward`：+ → −，`message = "A→B"`
   - `CrossingDirection::Reverse`：− → +，`message = "B→A"`
   - 落在线上（叉积 == 0）或仅触线不改侧：不记穿越。
6. **去重**：**只靠状态机发过渡/阈值事件**，不做毫秒级 cooldown。ROI：仅 `outside → inside`。逗留：同一次进入只在达到时长时发一次。越线：每次完成侧向穿越发一次。计数：同一 track 在 IN 集合中时不再 `countIn++`。
7. **消失的轨迹**：输入列表里没有的 `trackId` 视为离开场景。ROI / 逗留清该 id 状态（不补 Exit 事件）。计数：从 occupancy / IN 集合摘掉，**不**补 OUT。越线：丢掉该 id 的上一帧位置，不发明穿越。
8. **`IRule` 不接收 `cv::Mat`**：`evaluate(tracks, RuleContext{frameId, timestamp, sourceId})`。规则不碰像素。Lost 轨迹若仍在向量里，用其预测框的脚点。
9. **eventId**：由 `RuleEngine` 分配，会话内单调递增、不复用；`reset()` 后从 1 再分配。规则返回的事件 `eventId` 可为 0。
10. **EventLog**：`LatestResult` 是 newest-wins，只把事件挂在 `PresentedFrame` 会丢。pipeline 另设有界 `EventLog`（默认 256，满则 DropOldest，带互斥，供测试与 Phase 8 快照）。`PresentedFrame.events` 仍是**本帧新事件**。
11. **空引擎**：`RuleEngine*` 为空或引擎内无规则时，行为与现在相同（无事件、新统计为 0）。生产 `CameraManager` 注入空的 `RuleEngine`（没有默认 ROI / 线——没配置就不应报警）。测试可注入具体规则或 FakeRule。
12. **模式切换 / start**：只在推理线程、下一次 `evaluate` 前 `RuleEngine::reset()`；`VisionPipeline::start()` 在启动 jthread **之前** reset 引擎并清空 EventLog。GUI 的 `setMode` 不得直接调规则。
13. **本阶段不做**：QML 事件页、ROI 绘制、SQLite、规则热更新、环境变量开关、把规则做成插件、独立 RuleWorker、伪造延迟/FPS。延迟只进 `PipelineStats`，不写 benchmark 可执行文件（那是 Phase 9）。

默认值：`LoiterConfig::loiterSeconds = 5.0`。`classIds` 为空表示所有类别。`EventLog` 容量 256。

放弃的备选：独立 RuleWorker（多一段队列和关机路径，规则本身很便宜）；用检测插件扩展规则（规格要求规则吃 Track，且检测器不知道跟踪器）；在 GUI 线程对 `latest()->tracks` 求值（GUI 禁止跑分析，且会丢帧事件）；时间窗 cooldown 叠在状态机上（V1 过渡事件已够，Phase 8 再加）；inside 用框中心或 IoU（脚点更贴近地面目标）。

## 推荐执行顺序

`P7-T01 → T02 → T03 → T04 → T05 → T06 → T07 → T08 → T09`

T04–T07 在 T03 之后理论上可并行（都只依赖几何原语 + `IRule`）。T08 在 T02 之后即可用 FakeRule 单测，不必等四条真规则。T09 会改 `InferenceWorker` / `VisionPipeline` / `StatsProbe`，必须串行且放在最后。单会话按上列顺序执行。

---

### P7-T01  VisionEvent 领域类型

**Task ID:** P7-T01

**Task Name:** VisionEvent 领域类型

**Goal:** 先有可编译、可断言的事件 / 类型枚举 / 规则上下文与引擎统计结构。本任务不写规则算法、不接管线。

**Files likely affected:**
- `CMakeLists.txt`（`visionlab_core` 加入 `core/VisionEvent.h`）
- `tests/CMakeLists.txt`（`tst_visionevent`，Windows PATH 列表加上 `visionevent`）

**New files:**
- `core/VisionEvent.h`
- `tests/VisionEventTest.cpp`

**Interfaces affected:** 新增（均在 `visionlab` 命名空间）。不改 `Track` / `Detection`。

```cpp
enum class EventType : std::uint8_t
{
    RoiIntrusion,
    LineCrossing,
    Loitering,
    Counting,
};

enum class CrossingDirection : std::uint8_t
{
    None,
    Forward,  // AB 左侧 → 右侧，message "A→B"
    Reverse,  // AB 右侧 → 左侧，message "B→A"
};

struct RuleContext
{
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
    std::string sourceId;
};

struct VisionEvent
{
    std::uint64_t eventId = 0;
    EventType type = EventType::RoiIntrusion;
    std::string ruleId;
    std::string sourceId;
    std::uint64_t trackId = 0;
    int classId = -1;
    std::string label;
    float confidence = 0.0F;
    cv::Rect box;
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
    std::string message;
    std::string snapshotRef;  // Phase 8 再填；V1 保持空
    CrossingDirection direction = CrossingDirection::None;
    std::uint64_t countIn = 0;
    std::uint64_t countOut = 0;
    std::size_t occupancy = 0;
};

struct RuleEngineStats
{
    std::size_t ruleCount = 0;
    std::size_t enabledRules = 0;
    std::uint64_t eventsEmitted = 0;
    double lastEvaluateLatencyMs = 0.0;
};
```

`VisionEvent.h` 纯领域，不依赖 Qt。`core` 仍不链接 Qt。不含 `cv::Mat`。

**Implementation outline:**
- 头文件放 `core/`，与 `Track.h` 同层。
- 不改 `PresentedFrame`（T09 再加 `events`）。
- 不引入 `IRule`。
- 各规则的 `*Config` 放在各自头文件（T04–T07），本任务不预建神类配置头。

**Risks:**
- 把 `cv::Mat` / 快照图放进 `VisionEvent` 会让 EventLog 与 LatestResult 变重；`snapshotRef` 只保留空字符串占位。
- `eventId` 用 `int` 长时间运行会溢出；必须 `uint64_t`。
- 在领域类型里引入 Qt 会破坏 core/QML 隔离。

**Required tests:**
- 默认 `VisionEvent`：`eventId==0`，`type==RoiIntrusion`，`direction==None`，`snapshotRef.empty()`，`trackId==0`。
- 默认 `RuleContext`：`frameId==0`，`sourceId.empty()`。
- 默认 `RuleEngineStats` 全 0。
- `visionlab_core` 目标仍不链接 Qt。

**Acceptance criteria:**
- 后续任务只 `#include "core/VisionEvent.h"`，不再发明另一套事件字段名。
- 生产管线零行为变化。

**Dependencies:** 无。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(core): add VisionEvent domain types for the rule engine`

---

### P7-T02  IRule 契约 + FakeRule + 算法文档

**Task ID:** P7-T02

**Task Name:** IRule 契约与规则说明

**Goal:** 把规则扩展点写成可编译契约，并用短文档锁死 inside 判定、四条状态机、去重和管线插入点。本任务不实现四条真规则，不改 InferenceWorker。

**Files likely affected:**
- `CMakeLists.txt`（新增 `visionlab_analytics`：本任务可以是仅头文件静态库，`LINKER_LANGUAGE CXX`；链 `visionlab_core`，不链 Qt）
- `tests/CMakeLists.txt`（`tst_irule`，Windows PATH 加上 `irule`）

**New files:**
- `analytics/IRule.h`
- `tests/fakes/FakeRule.h`
- `tests/IRuleTest.cpp`
- `docs/analytics/rule-engine.md`

**Interfaces affected:**

```cpp
class IRule {
public:
    virtual ~IRule() = default;
    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual EventType eventType() const = 0;
    virtual std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context) = 0;
    virtual void reset() = 0;
};
```

线程约定（写进头注释与文档）：同一实例的 `evaluate` / `reset` 不承诺可并发；只由推理线程调用（`start()` 之前的 reset 除外）。`evaluate` 不得修改入参 Track，不得访问像素。启用/禁用由 `RuleEngine` 持有，**不**放进 `IRule`。

`FakeRule`（仅测试）：构造 `explicit FakeRule(std::string id = "fake")`；`id()` 返回该字符串，`name()=="fake"`，`eventType()==RoiIntrusion`；对每个输入 track 产一条事件（拷贝 `trackId` / `classId` / `label` / `confidence` / `box`，填 `ruleId` / `sourceId` / `frameId` / `timestamp` / `message=="fake"`）；空输入返回 `{}`；`reset` 把内部计数清零；另提供 `resetCount()`（仅测试头）供 T09 观察模式切换。用来测引擎与管线接线，不冒充几何规则。T08 挂两条 FakeRule 时用不同 id（如 `"fake"` / `"fake2"`）。

**Implementation outline:**
- `docs/analytics/rule-engine.md` 必须写清：脚点；`pointPolygonTest>=0` 含边；ROI 只发进入；越线要侧向变号 + 线段相交；逗留按 `RuleContext.timestamp` 秒阈值；计数 IN=Forward / OUT=Reverse、occupancy 为 IN 集合大小、消失不补 OUT；无时间 cooldown；插入点在 InferenceWorker track 之后 render 之前；为何不单独开规则线程；生产默认空引擎；EventLog 256 DropOldest 的理由（LatestResult 会丢稀疏事件）；V1 不画 ROI、无 QML、无 SQLite。
- 四条规则的状态机在文档里用短列表写死，T04–T07 只实现文档，不再改 `IRule` 签名。
- 能力列表不要预留 plugin / QML 字段。
- 不改检测插件、不改 `ITracker`。

**Risks:**
- `evaluate` 若吃 `FramePacket` 会暗示规则可以写 `image`，破坏只读契约。
- 文档若留「以后再决定脚点还是中心」，T04/T06 会分叉。
- FakeRule 若做几何判定，T09 会误以为 ROI 已实现。

**Required tests:**
- FakeRule：两条 Track → 两条事件，id/box/label 拷自 Track，`ruleId=="fake"`。
- 空 tracks → 空事件。
- `reset` 后 `resetCount()==1`。
- 头文件可被测试 include，且 `visionlab_analytics` 不链接 Qt。

**Acceptance criteria:**
- T04–T08 只实现文档里的状态机，不再改 `IRule` 签名。
- 生产路径零行为变化。

**Dependencies:** T01。接口变更：是。并发风险：否（只定契约）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add IRule contract and rule-engine design notes`

---

### P7-T03  几何原语：脚点、多边形、有向越线

**Task ID:** P7-T03

**Task Name:** 规则几何原语

**Goal:** 把四条规则依赖的几何做成可单独测的函数。本任务不写规则状态机，不接管线。

**Files likely affected:**
- `CMakeLists.txt`（`visionlab_analytics` 加入源文件）
- `tests/CMakeLists.txt`（`tst_rulegeometry`，Windows PATH 加上 `rulegeometry`）

**New files:**
- `analytics/RuleGeometry.h`
- `analytics/RuleGeometry.cpp`（若头文件无法 inline 干净；允许纯头文件）
- `tests/RuleGeometryTest.cpp`

**Interfaces affected:**

```cpp
cv::Point2f footPoint(const cv::Rect& box);
// 空框返回 (0,0)。否则 (x + width/2, y + height)，float。

bool pointInPolygon(const cv::Point2f& point,
                    const std::vector<cv::Point2f>& polygon);
// polygon.size()<3 → false。内部用 cv::pointPolygonTest，>=0 为 true（含边）。

std::vector<cv::Point2f> polygonFromRect(const cv::Rect& rect);
// 四顶点：TL, TR, BR, BL。空矩形仍返回 4 点（可能重合）。

bool segmentsIntersect(const cv::Point2f& p1, const cv::Point2f& p2,
                       const cv::Point2f& q1, const cv::Point2f& q2);
// 含端点相触。零长度线段（p1==p2 或 q1==q2）返回 false。

int lineSide(const cv::Point2f& point,
             const cv::Point2f& a, const cv::Point2f& b);
// 叉积符号：+1 左，-1 右，0 共线或 AB 零长度。

CrossingDirection classifyCrossing(const cv::Point2f& previous,
                                   const cv::Point2f& current,
                                   const cv::Point2f& a,
                                   const cv::Point2f& b);
// 上一侧与本侧异号且都不为 0，且 prev-curr 与 A-B 相交 → Forward 或 Reverse。
// 否则 None。
```

只用 OpenCV，不引入新库。检测器 / 跟踪器不得 include 本头。

**Implementation outline:**
- 叉积用 float，近零用绝对阈值（例如 `1e-6`）当作共线，避免 1px 抖成穿越。
- `classifyCrossing` 是 T05/T07 的唯一越线入口，禁止在规则里再手写一套。
- 不在本任务写 ROI / 逗留状态机。

**Risks:**
- 整数 `cv::Rect` 与 float 脚点往返会偏 0.5px；测试用明确矩形，不要像素级咬死 OpenCV 轮廓。
- 端点相触若算穿越，目标贴线抖动会刷事件——相触只作为 `segmentsIntersect` 的几何事实，`classifyCrossing` 仍要求两侧非 0。
- AB 重合点会让 `lineSide` 全 0，必须返回 None。

**Required tests:**
- `footPoint`：`(10,10,20,30)` → `(20, 40)`。
- 点在正方形内 / 边上 / 外：对应 true / true / false。
- `<3` 顶点：`pointInPolygon` false。
- `polygonFromRect` 四点围成的多边形包含矩形中心。
- 两段交叉相交 true；平行不相交 false；仅端点相触 true；零长度 false。
- 从 AB 左侧走到右侧且线段相交：`Forward`；反向 `Reverse`；平行于线移动 `None`；走到线上停住 `None`。

**Acceptance criteria:**
- T04–T07 只组合这些函数，不再手写点在多边形或叉积。
- 不改检测 / 插件 / 管线。

**Dependencies:** T01（`CrossingDirection`）。接口变更：是（analytics 内部 API）。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add foot-point and directed-crossing geometry`

---

### P7-T04  RoiIntrusionRule

**Task ID:** P7-T04

**Task Name:** ROI 闯入规则

**Goal:** 多边形 ROI 上实现 outside→inside 过渡事件。不接管线、不画框。

**Files likely affected:**
- `CMakeLists.txt`
- `tests/CMakeLists.txt`（`tst_roiintrusion`，Windows PATH 加上 `roiintrusion`）

**New files:**
- `analytics/RoiIntrusionRule.h`
- `analytics/RoiIntrusionRule.cpp`
- `tests/RoiIntrusionRuleTest.cpp`

**Interfaces affected:**

```cpp
struct RoiConfig {
    std::string ruleId = "roi";
    std::vector<cv::Point2f> polygon;
    std::vector<int> classIds;  // 空 = 全部类别
};

class RoiIntrusionRule final : public IRule {
public:
    explicit RoiIntrusionRule(RoiConfig config);
    std::string id() const override;
    std::string name() const override;       // "roi-intrusion"
    EventType eventType() const override;    // RoiIntrusion
    std::vector<VisionEvent> evaluate(const std::vector<Track>&, const RuleContext&) override;
    void reset() override;
};
```

算法（必须与文档一致）：
1. `polygon.size()<3`：本帧返回 `{}`（不抛）。
2. 对每条 track：若 `classIds` 非空且 `classId` 不在其中，视为 outside（并清该 id 的 inside 标记）。
3. `inside = pointInPolygon(footPoint(track.box), polygon)`。
4. 若本帧 inside 且上一帧该 `trackId` 不是 inside → 发一条事件（`message=="intrusion"`，填 track 字段与 context）。
5. 本帧 still inside：不发。outside：不发 Exit。
6. 本帧列表中没有的 `trackId`：丢掉 inside 标记（再出现算新进入）。
7. `reset()` 清空全部 inside 标记。

**Implementation outline:**
- 内部 `unordered_set<uint64_t> m_inside`。
- 事件 `eventId` 保持 0，交给 T08 分配。
- 合成 Track，不读真实视频。不链接 Qt。

**Risks:**
- 用框中心会让测试在「脚在线外、头在内」时误绿——测试必须把脚点明确放在内外。
- 每帧都发事件会直接违反 DoD；stay 用例必须断言空。
- 类别过滤失败会让运动块（`kMotionClassId`）误报；测一条 classId 不匹配不发事件。

**Required tests:**
- 脚点在外：无事件。
- 外→内：恰好一条，`type==RoiIntrusion`，`trackId` 正确，`message=="intrusion"`。
- 内停留第三帧：无事件。
- 内→外：无事件。
- 再进入：又一条。
- 多边形 `<3` 点：始终空。
- `classIds={0}` 而 track `classId==1`：即使脚点在内也不发。
- 轨迹从列表消失再出现在内：算新进入，发事件。
- `reset` 后仍在 ROI 内的下一帧：再发一次（视为新会话进入）。

**Acceptance criteria:**
- 检测器 / ByteTrack 源码零修改。
- 状态机与文档一致。

**Dependencies:** T02, T03。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add ROI intrusion rule with enter-only events`

---

### P7-T05  LineCrossingRule

**Task ID:** P7-T05

**Task Name:** 有向越线规则

**Goal:** 用相邻两帧脚点检测真正穿越，区分 A→B 与 B→A。不接管线。

**Files likely affected:**
- `CMakeLists.txt`
- `tests/CMakeLists.txt`（`tst_linecrossing`，Windows PATH 加上 `linecrossing`）

**New files:**
- `analytics/LineCrossingRule.h`
- `analytics/LineCrossingRule.cpp`
- `tests/LineCrossingRuleTest.cpp`

**Interfaces affected:**

```cpp
struct LineConfig {
    std::string ruleId = "line";
    cv::Point2f a{};
    cv::Point2f b{};
    std::vector<int> classIds;
};

class LineCrossingRule final : public IRule {
public:
    explicit LineCrossingRule(LineConfig config);
    std::string id() const override;
    std::string name() const override;       // "line-crossing"
    EventType eventType() const override;    // LineCrossing
    std::vector<VisionEvent> evaluate(const std::vector<Track>&, const RuleContext&) override;
    void reset() override;
};
```

算法：
1. A==B：本帧 `{}`。
2. 类别过滤同 T04。
3. 每个仍在列表中的 track：取本帧脚点。若有上一帧脚点，`classifyCrossing(prev, curr, a, b)`；非 None 则发事件，`direction` 填入，`message` 为 `"A→B"` 或 `"B→A"`。
4. 本帧首次出现的 track：只记录位置，不发事件。
5. 列表中消失的 track：删除上一帧位置。
6. `reset()` 清空位置表。

**Implementation outline:**
- 内部 `unordered_map<uint64_t, cv::Point2f> m_lastFoot`。
- 禁止用「框与线段相交」当穿越（规格点名禁止）。
- 合成水平/垂直移动序列，坐标用整数框换算脚点，避免浮点故事。

**Risks:**
- 只用当前框是否压线会在贴线停留时每帧报警。
- 平行于线的移动若因脚点量化抖过线，阈值必须靠 `classifyCrossing` 的共线带。
- 与 T07 重复实现穿越会分叉——本任务只调 T03。

**Required tests:**
- 从线左侧走到右侧且路径与 AB 相交：一条 Forward，`message=="A→B"`。
- 反向：一条 Reverse，`message=="B→A"`。
- 平行于线、不相交：无事件。
- 走到线上停住（侧变为 0）：无事件。
- 同一 track 连续两帧停在同一侧：无事件。
- 类别不匹配：无事件。
- 消失后再在对侧出现（中间无连续路径）：无事件（不跨消失发明穿越）。
- `reset` 后需要重新积累上一帧位置。

**Acceptance criteria:**
- 方向可测、可区分。
- 不改跟踪器。

**Dependencies:** T02, T03。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add directed line-crossing rule`

---

### P7-T06  LoiteringRule

**Task ID:** P7-T06

**Task Name:** 逗留规则

**Goal:** 脚点连续在 ROI 内达到配置秒数后发一次事件。处理离开、再进入、丢失、reset。

**Files likely affected:**
- `CMakeLists.txt`
- `tests/CMakeLists.txt`（`tst_loitering`，Windows PATH 加上 `loitering`）

**New files:**
- `analytics/LoiteringRule.h`
- `analytics/LoiteringRule.cpp`
- `tests/LoiteringRuleTest.cpp`

**Interfaces affected:**

```cpp
struct LoiterConfig {
    std::string ruleId = "loiter";
    std::vector<cv::Point2f> polygon;
    std::vector<int> classIds;
    double loiterSeconds = 5.0;
};

class LoiteringRule final : public IRule {
public:
    explicit LoiteringRule(LoiterConfig config);
    std::string id() const override;
    std::string name() const override;       // "loitering"
    EventType eventType() const override;    // Loitering
    std::vector<VisionEvent> evaluate(const std::vector<Track>&, const RuleContext&) override;
    void reset() override;
};
```

算法：
1. 多边形非法或 `loiterSeconds<=0`：返回 `{}`。
2. 对每个通过类别过滤且 inside 的 track：若无进入时刻，记 `context.timestamp` 为进入时刻，`emitted=false`。若已进入且未 emitted 且 `(context.timestamp - entered) >= loiterSeconds` → 发一条（`message=="loitering"`）并 `emitted=true`。已 emitted 且仍 inside：不发。
3. outside 或类别不匹配：清该 id 的进入时刻与 emitted。
4. 列表中消失：清该 id（不补事件，即使已接近阈值）。
5. Lost 但仍在列表且脚点仍 inside：计时继续（用预测框）。
6. 离开后再进入：新的进入时刻，可再发一次。
7. `reset()` 清空全部计时。

时长必须用 `RuleContext.timestamp`，禁止用帧数（帧率不稳）。测试用手工时间戳，不要 sleep。

**Risks:**
- 用帧计数在丢帧时会提前/推迟触发。
- 达到阈值后每帧再发会刷屏。
- 消失后仍保留计时，会导致「人已走、规则还在计」；必须删状态。

**Required tests:**
- 在内 4s（阈值 5s）：无事件。
- 第 5s 那一帧：恰好一条。
- 继续停在内：不再发。
- 3s 时离开：无事件；再进入后重新从 0 计，满 5s 才发。
- 再进入满阈值：第二条。
- 列表中途去掉该 track：无事件，再给一个 inside 的新序列需重新满阈值。
- `reset` 清计时。
- 手工时间戳，测试中禁止 `sleep`。

**Acceptance criteria:**
- 与 ROI 闯入是两个类、两份测试；逗留不复用 `RoiIntrusionRule` 的 inside 集合（各自维护）。
- 不接管线。

**Dependencies:** T02, T03。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add loitering rule with timestamp threshold`

---

### P7-T07  CountingRule

**Task ID:** P7-T07

**Task Name:** 过线计数规则

**Goal:** 在有向越线上做 IN / OUT 计数与 occupancy。同一 track 在 IN 集合中不重复 IN。消失不补 OUT。

**Files likely affected:**
- `CMakeLists.txt`
- `tests/CMakeLists.txt`（`tst_counting`，Windows PATH 加上 `counting`）

**New files:**
- `analytics/CountingRule.h`
- `analytics/CountingRule.cpp`
- `tests/CountingRuleTest.cpp`

**Interfaces affected:**

```cpp
struct CountConfig {
    std::string ruleId = "count";
    cv::Point2f a{};
    cv::Point2f b{};
    std::vector<int> classIds;
};

class CountingRule final : public IRule {
public:
    explicit CountingRule(CountConfig config);
    std::string id() const override;
    std::string name() const override;       // "counting"
    EventType eventType() const override;    // Counting
    std::vector<VisionEvent> evaluate(const std::vector<Track>&, const RuleContext&) override;
    void reset() override;
    std::uint64_t countIn() const;
    std::uint64_t countOut() const;
    std::size_t occupancy() const;          // IN 集合大小
};
```

算法：
1. 用与 T05 相同的上一帧脚点 + `classifyCrossing`。
2. Forward 且 `trackId` **不在** IN 集合：`countIn++`，加入集合，发 Counting 事件（`direction==Forward`，`message=="IN"`，填当前 `countIn/countOut/occupancy`）。
3. Reverse 且 **在** IN 集合：`countOut++`，移出集合，发事件（`message=="OUT"`）。
4. Forward 但已在 IN 集合：不发、不 `countIn++`（防抖/双计）。
5. Reverse 但不在 IN 集合：不发、不 `countOut++`（没进过线的出场忽略）。
6. 列表中消失：从 IN 集合摘掉，occupancy 下降，**不** `countOut++`、不发 OUT。
7. `reset()`：计数器与集合与位置表全清。

不组合 `LineCrossingRule` 对象（避免双重状态）；只复用 T03 函数。两条规则可同时挂在引擎上，互不共享内部 map。

**Risks:**
- 「避免双计」理解成「一生只计一次」会让往返过门失败——锁的是 IN 集合滞后，不是终身禁计。
- 消失补 OUT 会把「走出画面」当成出门。
- 与 T05 各写一套穿越判定会不一致。

**Required tests:**
- 一次 Forward：`countIn==1`，`occupancy==1`，一条 IN 事件。
- 再一次同向抖过线但仍在 IN 集：计数不变。
- Reverse：`countOut==1`，`occupancy==0`，一条 OUT。
- 同一 track IN→OUT→IN：`countIn==2`，`countOut==1`。
- 两条 track 先后 IN：`countIn==2`，`occupancy==2`。
- IN 之后从列表删除：`occupancy==0`，`countOut` 仍为 0，无新事件。
- `reset` 后计数为 0。

**Acceptance criteria:**
- occupancy 可从规则实例读到，也出现在事件字段里。
- 不改 ByteTrack。

**Dependencies:** T02, T03。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add directional counting with occupancy`

---

### P7-T08  RuleEngine

**Task ID:** P7-T08

**Task Name:** RuleEngine 注册、启用与求值

**Goal:** 拥有规则列表，只对启用项求值，分配 `eventId`，汇总统计。本任务可用 FakeRule 测完；不接管线。

**Files likely affected:**
- `CMakeLists.txt`
- `tests/CMakeLists.txt`（`tst_ruleengine`，Windows PATH 加上 `ruleengine`）

**New files:**
- `analytics/RuleEngine.h`
- `analytics/RuleEngine.cpp`
- `tests/RuleEngineTest.cpp`

**Interfaces affected:**

```cpp
class RuleEngine {
public:
    bool addRule(std::unique_ptr<IRule> rule);
    // nullptr 或空 id 或 id 重复 → false，不接管所有权（重复时销毁传入对象或在失败路径 drop unique_ptr）。
    // 成功则默认 enabled=true。

    bool setEnabled(std::string_view ruleId, bool enabled);
    bool isEnabled(std::string_view ruleId) const;
    std::size_t size() const;

    std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context);

    void reset();
    RuleEngineStats stats() const;
};
```

行为：
1. `evaluate`：按注册顺序对 enabled 规则调用；某条规则抛 `cv::Exception` 则记下日志、该规则本帧贡献 `{}`，**继续**其它规则，引擎不把异常甩给 worker。
2. 拼接后的事件：按顺序把 `eventId` 设为 1,2,3…（会话累计，不是每帧从 1）。`eventsEmitted` 为已分配的累计个数。
3. 规则返回里已有非 0 `eventId` 也覆盖为引擎分配值（Fake/真规则都应被覆盖）。
4. `lastEvaluateLatencyMs` 为这次 `evaluate` 墙钟（含所有规则）。
5. `reset()`：对每条规则 `reset()`，id 生成器回到 1，`eventsEmitted=0`，`lastEvaluateLatencyMs=0`；**不**删除规则，不改变 enabled。
6. 同一实例不承诺可并发 `evaluate`/`addRule`。无内部互斥（T09 的 EventLog 才有锁）。
7. 无规则：`evaluate` 返回 `{}`，耗时可记 0。

**Implementation outline:**
- 内部结构体 `{ unique_ptr<IRule> rule; bool enabled; }` 的 vector。
- `stats().ruleCount = size()`，`enabledRules` 为 enabled 个数。
- 测试同时挂两条 FakeRule：`FakeRule{"fake"}` 与 `FakeRule{"fake2"}`（构造已在 T02 锁定）。
- 本任务不创建 EventLog。

**Risks:**
- 一条规则抛异常若中止整个 evaluate，其它规则会哑火；必须隔离。
- 在引擎里加锁再让 worker 同线程 evaluate 会无意义地复杂化；锁留给 EventLog。
- 重复 id 若覆盖旧规则，测试与 Phase 8 配置会难以推理——失败更安全。

**Required tests:**
- 空引擎：事件空，`ruleCount==0`。
- 一条 FakeRule + 一条 track：一条事件，`eventId==1`，`eventsEmitted==1`。
- 第二帧再一条：`eventId==2`。
- 两条不同 id 的 FakeRule：一帧两条事件，id 为 1 和 2，顺序为注册顺序。
- `setEnabled(id,false)` 后该规则不再出事件；再启用恢复。
- 重复 id：`addRule` false，`size` 仍为 1。
- `reset` 后下一事件 `eventId==1`，且 FakeRule `resetCount` 增加。
- 禁用不影响 `size()`，只减 `enabledRules`。

**Acceptance criteria:**
- 引擎不 `#include` 检测器 / 跟踪器实现头（可以 include `core/Track.h`）。
- 无 Qt。

**Dependencies:** T02（T04–T07 可选，本任务不强制）。接口变更：是。并发风险：否（单线程单元测试）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add RuleEngine with enable flags and event ids`

---

### P7-T09  管线插入：EventLog、统计、生产空引擎

**Task ID:** P7-T09

**Task Name:** 在 InferenceWorker 接入 RuleEngine

**Goal:** 推理线程在 track 之后求值规则；本帧事件进入 `PresentedFrame.events`；历史进入有界 EventLog；指标进入 `PipelineStats`。生产注入空 `RuleEngine`。模式变化时在推理线程 `reset`。本任务仍不画 ROI。

**Files likely affected:**
- `core/PresentedFrame.h`（增加 `std::vector<VisionEvent> events`）
- `core/PipelineStats.h`（增加 `eventsEmitted` / `enabledRules` / `avgRuleLatencyMs`）
- `pipeline/EventLog.h` / `.cpp`（新）
- `pipeline/InferenceWorker.h` / `.cpp`
- `pipeline/VisionPipeline.h` / `.cpp`
- `pipeline/StatsProbe.h` / `.cpp`
- `utilities/CameraManager.cpp`（组装时 `make_unique<RuleEngine>()`，注释：生产无默认 ROI）
- `utilities/CameraManager.h`（注释：pipeline 拥有 RuleEngine 与 EventLog）
- `docs/analytics/rule-engine.md`（补生产接线、reset 时机、EventLog、与插件/跟踪的边界）
- `tests/InferenceWorkerTest.cpp`
- `tests/VisionPipelineTest.cpp`
- `tests/StatsProbeTest.cpp`
- `tests/PipelineLifecycleTest.cpp`（start/stop 后 EventLog 空、规则 reset）
- `CMakeLists.txt`（`visionlab_pipeline` 链接 `visionlab_analytics`；`EventLog` 加入 pipeline 目标）
- `tests/CMakeLists.txt`（`tst_eventlog`，Windows PATH 加上 `eventlog`）

**New files:**
- `pipeline/EventLog.h`
- `pipeline/EventLog.cpp`
- `tests/EventLogTest.cpp`

**Interfaces affected:**

`PresentedFrame` 增加 `events`，默认空。`detections` / `tracks` 保留。

`PipelineStats` 增加：

```cpp
std::uint64_t eventsEmitted = 0;
std::size_t enabledRules = 0;
double avgRuleLatencyMs = 0.0;
```

`EventLog`：

```cpp
class EventLog {
public:
    explicit EventLog(std::size_t capacity = 256);
    void push(const std::vector<VisionEvent>& events);  // 逐条追加，超容量 pop_front
    std::vector<VisionEvent> snapshot() const;          // 值拷贝
    void reset();
    std::size_t size() const;
};
```

内部一把 `mutex`，与 `StatsProbe` 相同：推理线程写，任意线程 snapshot。

`StatsProbe::onRuled(const RuleEngineStats& stats, double ruleLatencyMs)`：同一把 `m_mutex` 覆盖写入 `eventsEmitted` / `enabledRules`，单独 `LatencyWindow` 记规则耗时。`snapshot()` 填 `avgRuleLatencyMs`。`reset()` 清零新字段。无规则时 worker 不调用 `onRuled`，新字段保持 0。

`InferenceWorker` 构造在现有默认参数**之后**增加 `RuleEngine* rules = nullptr`、`EventLog* events = nullptr`。旧测试不用改调用。

求值时机：
- tracker 块结束之后、render 之前。
- 无 tracker 时 `presented.tracks` 为空，规则仍可 evaluate（得到空或 Fake 空）。
- `m_rules` 非空且 `m_mode` 非空：mode 变化时**先** `m_tracker->reset()`（若有）**再** `m_rules->reset()`，然后 update / evaluate。
- `evaluate` 包在 try/catch（`cv::Exception`，与 detect/track 相同）：异常则 `presented.events.clear()`，打日志，worker 不得退出。引擎内部已隔离单条规则异常；此处防引擎本身抛出。
- 成功后：`presented.events = ...`；若 `m_eventLog` 非空则 `push`；`onRuled`。
- `IRule*` / 引擎为空：不调用，`events` 保持空。

`VisionPipeline` 构造增加第五参数 `std::unique_ptr<RuleEngine> rules = {}`。`EventLog` 是**值成员**（不是 `unique_ptr`），避免与 `EventLog::reset()` 混淆。成员声明：`m_rules` 与 `m_eventLog` 活得比 worker 长（先声明，`joinWorkers` 仍先 `reset` worker）。`start()` 在启动 jthread 前：`m_tracker->reset()`（若有）、`m_rules->reset()`（若有）、`m_eventLog.reset()` 清空日志。把 `m_rules.get()` 与 `&m_eventLog` 传给 worker。提供 `std::vector<VisionEvent> recentEvents() const` → `m_eventLog.snapshot()`。

禁止在 `VisionPipeline::setMode` 里直接 `m_rules->reset()`。

`CameraManager::assembleFromPlugins`：

```cpp
std::make_unique<VisionPipeline>(
    std::move(source), std::move(detectors),
    VisionPipeline::kDefaultQueueCapacity,
    std::make_unique<ByteTrackTracker>(),
    std::make_unique<RuleEngine>());
```

不注册任何 ROI / 线。不加 QML property，不加 `VISIONLAB_RULES` 环境变量。

Grep：`detectors/` 与 `plugins/` 与 `tracking/` 不得出现 `IRule` / `RuleEngine` / `VisionEvent`（`Track.h` 除外的 analytics 依赖）。

**Implementation outline:**
- 注释改成：跟踪与规则都在本线程、detect 与 render 之间，规则在 track 之后。
- 本任务不改 DetectionRenderer / TrackRenderer / QML。
- `onInferred` 语义不变：规则耗时不计入 `avgInferenceLatencyMs`。
- 禁止在测试里写死「必须 < X ms」。
- 工作区里未提交的 inference 换行改动不要 `git add`。

**Risks:**
- GUI `setMode` 与推理 `evaluate` 并发：规则内部无锁。必须把 reset 放在推理线程。
- Worker 持有 `RuleEngine*` 而 pipeline 先毁引擎：UAF。所有权只在 pipeline。
- 只把事件放在 PresentedFrame：UI 丢帧即丢事件。EventLog 是本阶段为 Phase 8 留下的最小邮箱。
- 把第五个构造参数插到中间会弄断现有四参数调用。
- 生产若默认塞一个屏幕大 ROI，相机会无配置报警。

**Required tests:**
- EventLog：push 3 条 size==3；push 300 条容量 256 时 size==256 且 snapshot 含最后一条、不含最早那条；`reset` 后空；并发 push/snapshot 不崩（可短跑，与 StatsProbe 的 concurrent 用例同级）。
- StatsProbe：空探针新字段为 0；`onRuled` 两次 `eventsEmitted` 取最后一次传入的累计值；`avgRuleLatencyMs` 对 10 和 30 为 20；`reset` 后新字段为 0；旧用例仍绿。
- InferenceWorker 无规则：`presented.events.empty()`，detections/tracks 与现在一致。
- InferenceWorker + FakeTracker + FakeRule：有检测时 `events.size()==tracks.size()`，`recent` 路径：worker 的 EventLog size>0。
- 空 detections：FakeRule 空，events 空。
- 切换 mode：FakeRule `resetCount` 增加（与 tracker reset 同一帧逻辑）。
- VisionPipeline + FakeDetector + FakeTracker + FakeRule：若干帧后 `latest()->events` 非空，且 `recentEvents()` 非空。
- VisionPipeline 无规则指针（默认）：`events` 空，行为与 Phase 6 相同。
- start/stop/start：EventLog 空，下一事件 `eventId==1`（引擎 reset）。
- 可选一条接近生产的装配：ByteTrackTracker + 空 RuleEngine + FakeDetector，不要求 GPU，只断言不崩且 `events.empty()`。
- 现有 InferenceWorker / VisionPipeline / overload / lifecycle / cameramanager 测试仍绿。
- 全量 ctest：默认 CPU 构建，无 GPU 项不得无故变红。

**Acceptance criteria（对照 Phase 7 DoD）：**
- `IRule` 存在（T02）。
- RuleEngine 不依赖检测器/跟踪器实现（T08）。
- ROI / 越线 / 逗留 / 计数有单测（T04–T07）；本任务把引擎接到管线。
- 事件去重由规则状态机保证，管线不每帧复制状态报警。
- 规则可配置（构造期 Config；生产为空，测试注入）。
- GUI 不跑 `evaluate`；不新开线程。
- UI 仍不展示事件列表（Phase 8）；本阶段用测试读 `latest()->events` / `recentEvents()`。
- 文档写清四条状态机（T02 已写，本任务补接线）。

**Dependencies:** T08；管线 Fake 路径不强制 T04–T07，但本阶段收口前 T04–T07 应已绿。接口变更：是。并发风险：是（EventLog / StatsProbe / mode reset）。新第三方依赖：否。需测量：否（只记录，不设门槛）。

**Recommended Git commit message:** `feat(pipeline): evaluate rules after track and keep a bounded event log`

---

## 任务总览

| Task ID | 名称 | 依赖 | 接口变更 | 并发风险 | 新依赖 | 需测量 | 新测试 |
|---|---|---|---|---|---|---|---|
| P7-T01 | VisionEvent 领域类型 | 无 | 是 | 否 | 否 | 否 | 是 |
| P7-T02 | IRule + 规则文档 | T01 | 是 | 否 | 否 | 否 | 是 |
| P7-T03 | 脚点 / 多边形 / 越线原语 | T01 | 是 | 否 | 否 | 否 | 是 |
| P7-T04 | RoiIntrusionRule | T02, T03 | 是 | 否 | 否 | 否 | 是 |
| P7-T05 | LineCrossingRule | T02, T03 | 是 | 否 | 否 | 否 | 是 |
| P7-T06 | LoiteringRule | T02, T03 | 是 | 否 | 否 | 否 | 是 |
| P7-T07 | CountingRule | T02, T03 | 是 | 否 | 否 | 否 | 是 |
| P7-T08 | RuleEngine | T02 | 是 | 否 | 否 | 否 | 是 |
| P7-T09 | 管线 + EventLog + 空引擎 | T08 | 是 | 是 | 否 | 否 | 是 |

## Phase 7 DoD 对照

| DoD | 任务 |
|---|---|
| IRule abstraction exists | T02, T08 |
| RuleEngine is detector/tracker independent | T08, T09 Grep |
| ROI intrusion works | T04 |
| Line crossing works directionally | T05 |
| Loitering works | T06 |
| Counting works | T07 |
| Event duplication is controlled | T04–T07 状态机；文档锁定无 per-frame spam |
| Rules are configurable | T04–T07 Config；T08 enable；生产空引擎 |
| Tests cover geometry/state edge cases | T03–T07 |
| Pipeline remains responsive | T09 不新增 GUI 阻塞；不新开线程 |
| After implementation explain state machines | T02 文档 + T09 补接线 |

指定 Task ID 之前不要写实现代码。

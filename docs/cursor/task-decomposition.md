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

# Phase 8 — Qt/QML 监控台、规则绘制与事件存储 任务分解

对照 HEAD `ad3c19e`（`feat(pipeline): evaluate rules after track and keep a bounded event log`）。
Phase 0–7 实现已提交。Phase 7 未做单独「收口」提交，但不阻塞本阶段：
`IRule` / 四条规则 / `RuleEngine` / `EventLog` / 生产空引擎都在 `master` 上。

当前 UI 仍是单页：无边框窗口 + 标题栏中英切换 + 启停 + 三种检测模式 + `CameraView`。
QML 只吃 `image://camera/live`。`CameraView` 使用 `Image.PreserveAspectCrop`，
不能把叠加层坐标映回原始帧。`VisionController` 只有 `mode` / `running`。
没有 `QAbstractListModel`、没有 Events/Performance/Settings 页、没有 SQLite、
没有 `RuleSpec`、`RuleEngine` 不能 `clear()` / 列举规则。

工作区里 `AGENTS.md` 有未提交的任务报告约定，`inference/` 等文件有换行符噪声。
Phase 8 提交不要把这些文件卷进去。

## 当前架构

```
QML (Main / TitleBar / CameraView)
  → VisionController (mode / running)
      → CameraManager
          PluginManager → IVisionPlugin → unique_ptr<IDetector>
          VisionPipeline
            CaptureWorker (jthread)
              → BoundedQueue<FramePacket>  DropOldest
            InferenceWorker (jthread)
              → IDetector::detect
              → ITracker::update          （生产：ByteTrackTracker）
              → RuleEngine::evaluate      （生产：空引擎）
              → TrackRenderer / DetectionRenderer
              → LatestResult<PresentedFrame>
              → EventLog（256，DropOldest）
          CameraImageProvider → QML RGB
```

`PresentedFrame` 已有 `detections` / `tracks` / `events`。
`PipelineStats` 已有跟踪与规则字段。`VisionPipeline::recentEvents()` 可快照 EventLog。
本阶段不改 ByteTrack 算法、不改四条规则的状态机、不新开规则线程、不把规则做成检测插件。

## 锁定的设计决策

1. **四页壳 + 左侧导航**：Monitor / Events / Performance / Settings。保留现有无边框标题栏与中英按钮。窗口仍 1200×900，左侧导航 168px。启停是全局控件（标题栏右侧语言按钮左侧，或导航底部），切到事件页也能停摄像头。检测模式按钮只留在 Monitor。
2. **QML 不拥有领域对象**：不把 `RuleEngine*` / `IDetector*` / `ITracker*` / `IEventRepository*` 暴露给 QML。QML 只绑 `VisionController` 上的 `QAbstractListModel` / 属性。`CameraManager` 仍只管管线与帧。
3. **模型更新频率**：
   - `DetectionModel` / `TrackModel`：在 `CameraManager::notifyFrame`（已 Queued 回 GUI）里从 `latest()` 重建行。只拷领域字段，不拷 `cv::Mat`。
   - `EventModel`：对 `recentEvents()` 按 `eventId` 做增量 append，禁止每帧 `beginResetModel`。
   - `PerformanceModel`：`QTimer` **250ms** 调 `statsSnapshot()`，禁止每帧给 QML 发性能信号。
4. **画面映射**：Monitor 的 `Image.fillMode` 改为 `PreserveAspectFit`（letterbox）。坐标换算用纯函数 `rendering/Letterbox.h`（无 Qt）。禁止继续用 `PreserveAspectCrop` 画 ROI——裁切后鼠标点无法映回帧像素。
5. **ROI / 线画在 QML 叠加层**，不烧进 `TrackRenderer`。检测框与轨迹 ID 仍由现有 C++ renderer 画在 RGB 上。叠加层只画规则几何（多边形、有向线段）与绘制中的橡皮筋。
6. **规则变更必须停机**：`RuleEngine::evaluate` / `addRule` / `setEnabled` 与 GUI 并发不安全（Phase 7 契约）。绘制、增删、启用、改逗留秒数都只在 `!isRunning()` 时写入引擎。运行中叠加只读。`start()` 前 `CameraManager` 用当前 `RuleSpec` 列表 `clear` + `addRule` + `setEnabled`。不在本阶段做热更新。
7. **`RuleSpec` + `makeRule`**：UI 与持久会话状态只持有 `RuleSpec`（值类型）。`analytics/RuleFactory` 把它变成 `unique_ptr<IRule>`。不给 `IRule` 加 QML 字段。不改四条规则的状态机。
8. **设置里会重建管线的项**：backend / precision / deviceId / confidence / NMS / trackingEnabled。Apply 时若正在运行 → 返回 false，QML 提示先停止。成功路径：`stop` → 按会话设置 `assembleFromPlugins` → 重新注入 `RuleSpec`。本会话有效，**不写回环境变量**（启动仍读 `VISIONLAB_INFERENCE_*` 作为初值）。不提供 INT8（未实现）。
9. **关闭跟踪**：装配时 `ITracker` 传空。无 tracks 则规则看到空列表，不产生事件。画面回退 `DetectionRenderer`（现有 InferenceWorker 行为）。
10. **SQLite 用 Qt6::Sql 的 QSQLITE**，新库 `visionlab_storage`，不链 QML。GUI 线程禁止 `INSERT`/`SELECT`。`EventWriter` 用 `std::jthread` + `BoundedQueue<VisionEvent>`（容量 1024，DropOldest）。GUI 在 `notifyFrame` 里对 EventLog 快照做 `eventId` 增量 `enqueue`（不阻塞）。查询只在 writer 线程；结果 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 回 GUI。
11. **EventLog 仍是会话邮箱**：`LatestResult` 会丢帧事件，所以持久化与 EventModel 都读 `recentEvents()`，不读 `latest()->events`。EventLog 容量 256；本阶段不加大。跨 `start()` 时引擎 `eventId` 从 1 再分配——`EventModel` 必须在每次 `start()` 开始新 session（历史行保留，新行不按旧 session 的 eventId 去重）。
12. **`VisionEvent::timestamp` 仍是 `steady_clock`**，不改领域类型。SQLite 另存 `wall_utc_ms`（insert 时 `system_clock`）。Events 页显示墙钟。`snapshotRef` 本阶段保持空，不写 JPEG。
13. **保留策略**：表超过 **10000** 行则按 `rowid` 删最旧（DropOldest）。在 writer 线程、insert 之后调用。文档写明，不做成设置项。
14. **库路径**：`QStandardPaths::AppDataLocation` + `events.sqlite`（组织名/应用名已是 VisionLab）。测试用临时文件。打开失败：打日志，writer 入队即丢，应用不崩溃。
15. **本阶段不做**：qmltestrunner 全量 QML 套件（Phase 9）；规则 JSON 落盘；事件快照图；独立 RuleWorker；规则热更新；伪造 FPS/延迟；Material 换肤；把 `inference/` 换行噪声与未提交 `AGENTS.md` 塞进提交。

默认值：`LoiterConfig::loiterSeconds = 5.0`。`classIds` 空 = 全部类别。性能定时器 250ms。EventWriter 队列 1024。SQLite 上限 10000。Letterbox 用 `PreserveAspectFit` 语义（均匀缩放、居中、可能有黑边）。

放弃的备选：在 `TrackRenderer` 里画 ROI（无法交互、破坏检测/分析分离）；GUI 线程直接 `addRule`/`evaluate`（数据竞争）；用 `latest()->events` 喂 SQLite（Queued 回调会丢）；`PreserveAspectCrop` 上画线（坐标错）；Qt Sql 连接建在 GUI 线程；为过滤再打 DB（内存 `QSortFilterProxyModel` 即可）；规则热更新（Phase 7 明确推迟）；环境变量与 QSettings 双源写 backend。

## 推荐执行顺序

`P8-T01 → T02 → T03 → T04 → T05 → T06 → T07 → T08 → T09 → T10 → T11 → T12`

T01 / T02 / T03 无互相依赖，理论上可并行。T04–T07 只依赖各自的 C++ 契约。T08 依赖 T03 与 EventLog（已有）。T09 起改 QML，必须串行。T10 依赖 T01+T07+T09。T11 依赖 T05+T08+T09。T12 依赖 T05+T06+T07+T09。单会话按上列顺序执行。

---

### P8-T01  Letterbox 坐标映射

**Task ID:** P8-T01

**Task Name:** 帧像素 ↔ 控件坐标

**Goal:** 把 `PreserveAspectFit` 的几何做成可单测纯函数。本任务不改 QML、不画 ROI。

**Files likely affected:**
- `CMakeLists.txt`（`visionlab_rendering` 加入头/源）
- `tests/CMakeLists.txt`（`tst_letterbox`，Windows PATH 列表加上 `letterbox`）

**New files:**
- `rendering/Letterbox.h`
- `rendering/Letterbox.cpp`（若无法干净 inline 则需要）
- `tests/LetterboxTest.cpp`

**Interfaces affected:**

```cpp
struct Letterbox
{
    float offsetX = 0.F;  // 内容区左上角，相对 item
    float offsetY = 0.F;
    float contentW = 0.F;
    float contentH = 0.F;
    float scale = 1.F;    // item 像素 / 帧像素
};

Letterbox computeLetterbox(float itemW, float itemH, int frameW, int frameH);
// item 或 frame 任意边 <= 0 → 全 0。

bool itemToFrame(const Letterbox& box, float itemX, float itemY,
                 int frameW, int frameH, cv::Point2f& out);
// 点落在内容区外（含黑边）→ false，不写 out。
// 否则 out 为帧像素，可含小数；调用方画多边形时再 round。

bool frameToItem(const Letterbox& box, float frameX, float frameY,
                 cv::Point2f& out);
// box.scale==0 → false。
```

无 Qt。检测器 / 跟踪器 / 规则不得 include 本头。

**Implementation outline:**
- 缩放 `s = min(itemW/frameW, itemH/frameH)`，内容尺寸 `frameW*s` × `frameH*s`，居中。
- 与 Qt `Image.PreserveAspectFit` 一致（后续 T10 改 fillMode）。
- 不在本任务改 `CameraView.qml`。

**Risks:**
- 用 Crop 公式会让 T10 的点击永久偏。
- 整数帧尺寸与 float item 往返 0.5px；测试用整尺寸，断言误差 `< 1e-3` 或 round-trip 在 0.51px 内。

**Required tests:**
- 200×100 item、100×100 帧：`offsetX==50`，`offsetY==0`，`scale==1`，`contentW==contentH==100`。
- item (50,0) → frame (0,0)；item (150,100) → frame (100,100)；item (0,0) 黑边 → false。
- 正方形 item 装 16:9 帧：上下或左右黑边对称。
- 非法尺寸全 0，`itemToFrame` false。
- round-trip：帧内若干点 `frameToItem` 再 `itemToFrame`，误差小。

**Acceptance criteria:**
- T10 只调用这些函数，不再手写缩放。
- 生产路径零行为变化。

**Dependencies:** 无。接口变更：是（rendering 内部 API）。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(rendering): add PreserveAspectFit letterbox coordinate mapping`

---

### P8-T02  RuleSpec、RuleEngine::clear、makeRule

**Task ID:** P8-T02

**Task Name:** 规则规格与停机重建

**Goal:** UI 只编辑值类型 `RuleSpec`；停机时能清空引擎并按规格重建。不接管线、不改四条状态机。

**Files likely affected:**
- `analytics/RuleEngine.h` / `.cpp`（`clear`、`ruleIds`）
- `CMakeLists.txt`（`visionlab_analytics` 加入 RuleSpec / RuleFactory）
- `tests/CMakeLists.txt`（`tst_rulespec`，PATH 加上 `rulespec`）
- `tests/RuleEngineTest.cpp`（clear 后 size==0，再 add 仍可用）
- `docs/analytics/rule-engine.md`（补：配置经 RuleSpec；运行中禁止 mutate；Phase 8 停机 apply）

**New files:**
- `analytics/RuleSpec.h`
- `analytics/RuleFactory.h`
- `analytics/RuleFactory.cpp`
- `tests/RuleSpecTest.cpp`

**Interfaces affected:**

```cpp
enum class RuleKind : std::uint8_t
{
    RoiIntrusion,
    LineCrossing,
    Loitering,
    Counting,
};

struct RuleSpec
{
    std::string ruleId;
    RuleKind kind = RuleKind::RoiIntrusion;
    bool enabled = true;
    std::vector<cv::Point2f> polygon;
    cv::Point2f a{};
    cv::Point2f b{};
    std::vector<int> classIds;
    double loiterSeconds = 5.0;
};

// nullptr：空 id、未知 kind、ROI/Loiter 顶点 <3、Line/Count 的 A==B、loiterSeconds<=0。
std::unique_ptr<IRule> makeRule(const RuleSpec& spec);

class RuleEngine {
    void clear();                          // 丢掉全部规则；eventId 下次从 1；不抛
    std::vector<std::string> ruleIds() const; // 注册顺序
    // 现有 addRule / setEnabled / evaluate / reset / stats 不变
};
```

`makeRule` 按 kind 构造现有四类，拷贝对应 Config 字段。`enabled` 不放进 `IRule`，由引擎 `setEnabled` 负责。

**Implementation outline:**
- `clear()`：`m_entries.clear()`，然后与 `reset()` 相同地清计数器。
- `makeRule` 只 include 四条规则头；`RuleEngine.cpp` 仍不 include 它们。
- 不改 `RoiIntrusionRule` 等算法。

**Risks:**
- `clear` 若在运行中调用会与 `evaluate` 竞态——本任务单测只在单线程调用；文档写明仅停机。
- 把 `enabled` 塞进 `IRule` 会破坏 Phase 7 契约。

**Required tests:**
- 合法 ROI spec → `makeRule` 非空，`id()/eventType()` 正确；脚点外→内仍一条事件（可复用合成 Track）。
- 顶点 <3、空 id、A==B、`loiterSeconds==0` → nullptr。
- `addRule` 两条后 `ruleIds` 顺序一致；`clear` 后 `size()==0`、`ruleIds` 空；再 add + evaluate，新事件 `eventId==1`。
- `RuleEngineTest` 旧用例仍绿。

**Acceptance criteria:**
- 后续 UI 只存 `vector<RuleSpec>`，不再手 new 四类规则（测试 FakeRule 除外）。
- 检测器 / ByteTrack 源码零修改。

**Dependencies:** 无（Phase 7 已有 IRule）。接口变更：是。并发风险：否（单测）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(analytics): add RuleSpec factory and RuleEngine::clear`

---

### P8-T03  IEventRepository + SQLite

**Task ID:** P8-T03

**Task Name:** SQLite 事件仓储

**Goal:** 可插入、查询、过滤、超限裁剪的事件库。本任务不接管线、不写 QML。

**Files likely affected:**
- 根 `CMakeLists.txt`：`find_package(Qt6 REQUIRED COMPONENTS ... Sql)`；新静态库 `visionlab_storage`（链 `visionlab_core` + `Qt6::Sql`，不链 Quick/Qml）
- `tests/CMakeLists.txt`（`tst_eventrepository`，PATH 加上 `eventrepository`）

**New files:**
- `storage/IEventRepository.h`
- `storage/EventQuery.h`
- `storage/SqliteEventRepository.h`
- `storage/SqliteEventRepository.cpp`
- `tests/EventRepositoryTest.cpp`

**Interfaces affected:**

```cpp
struct EventQuery
{
    std::optional<EventType> type;
    std::uint64_t afterRowId = 0;
    std::size_t limit = 500;
};

struct StoredEvent
{
    std::int64_t rowId = 0;
    std::int64_t wallUtcMs = 0;
    VisionEvent event;
};

class IEventRepository {
public:
    virtual ~IEventRepository() = default;
    virtual bool open(const std::filesystem::path& dbPath) = 0;
    virtual bool insert(const VisionEvent& event, std::int64_t wallUtcMs) = 0;
    virtual std::vector<StoredEvent> query(const EventQuery& query) const = 0;
    virtual void prune(std::size_t maxRows) = 0;
    virtual std::size_t count() const = 0;
    virtual void close() = 0;
};
```

Schema（`IF NOT EXISTS`）：

```sql
CREATE TABLE events (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  wall_utc_ms INTEGER NOT NULL,
  pipeline_event_id INTEGER NOT NULL,
  type INTEGER NOT NULL,
  rule_id TEXT,
  source_id TEXT,
  track_id INTEGER,
  class_id INTEGER,
  label TEXT,
  confidence REAL,
  box_x INTEGER, box_y INTEGER, box_w INTEGER, box_h INTEGER,
  frame_id INTEGER,
  message TEXT,
  snapshot_ref TEXT,
  direction INTEGER,
  count_in INTEGER,
  count_out INTEGER,
  occupancy INTEGER
);
CREATE INDEX idx_events_type ON events(type);
```

`pipeline_event_id` 不是跨会话主键。`snapshot_ref` 存空串。

**Implementation outline:**
- 每个 `SqliteEventRepository` 实例只在**一个**线程上使用（T08 writer）。连接不要设成默认连接名抢全局。
- `open` 失败返回 false。`query` 在未打开时返回 `{}`。
- `prune(maxRows)`：`count() > maxRows` 时 `DELETE ... ORDER BY id ASC LIMIT extra`。
- 不引入独立 sqlite3 发行包。

**Risks:**
- 无 QSQLITE 驱动时测试会假红——`QSqlDatabase::isDriverAvailable("QSQLITE")` 为 false 则 `QSKIP`。
- 在 GUI 线程用这个类会违反 DoD；本任务测试在 Qt Test 主线程调用是允许的，T08 再搬到 jthread。
- 损坏文件：`open` false，不抛给调用方。

**Required tests:**
- 临时路径 `open` true；insert 3 条 `count==3`；`query` 默认按 `id` 升序。
- `EventQuery.type == LineCrossing` 只返回该类型。
- `afterRowId` + `limit` 分页。
- insert 10005 条后 `prune(10000)`，`count==10000`，最小 `pipeline_event_id` 已不是最早那条。
- 目录不可写或空路径：`open` false。
- 驱动不可用：`QSKIP`。

**Acceptance criteria:**
- `visionlab_storage` 不链 Qt Quick / Qml。
- `core/` 不 include `storage/`。

**Dependencies:** 无。接口变更：是。并发风险：否（单线程单测）。新第三方依赖：是（Qt6::Sql，随 Qt 安装）。需测量：否。

**Recommended Git commit message:** `feat(storage): add SQLite event repository with prune and query`

---

### P8-T04  DetectionModel + TrackModel

**Task ID:** P8-T04

**Task Name:** 检测 / 轨迹列表模型

**Goal:** QML 可绑的只读列表。本任务不改 QML 文件、不接 CameraManager。

**Files likely affected:**
- 根 `CMakeLists.txt` `qt_add_qml_module(appVisionLab)` 的 `SOURCES` 可先不加入（未接线）；测试可执行文件直接编译这两个 cpp
- `tests/CMakeLists.txt`（`tst_detectionmodel`、`tst_trackmodel`，PATH 加上二者）

**New files:**
- `models/DetectionModel.h` / `.cpp`
- `models/TrackModel.h` / `.cpp`
- `tests/DetectionModelTest.cpp`
- `tests/TrackModelTest.cpp`

**Interfaces affected:**

`DetectionModel` 与 `TrackModel` 均为 `QAbstractListModel`。

共同行为：`void setItems(...)` 在 GUI 线程调用；内部 `beginResetModel`/`endResetModel`（T04 行数通常 <100，可接受）。`rowCount` / `data` / `roleNames`。

Detection roles（字符串名给 QML）：`classId`, `label`, `confidence`, `x`, `y`, `width`, `height`。

Track roles：上列 + `trackId`, `state`（int，`TrackState`）, `age`。不把整个 `trajectory` 暴露为 QVariantList（轨迹已画在 RGB 上）。

```cpp
void DetectionModel::setDetections(const std::vector<Detection>&);
void TrackModel::setTracks(const std::vector<Track>&);
```

**Implementation outline:**
- 存 `QVector` 的 POD 拷贝（int/float/QString/QRect），不要持有 `cv::Mat`。
- 空 vector → 0 行。
- 不注册为 QML 可创建类型；T09 由 `VisionController` 当 QObject 子对象暴露。

**Risks:**
- 把 `PresentedFrame.rgb` 放进模型会每帧大拷贝。
- 从非 GUI 线程 `setItems` 会违反模型线程规则——头注释写明仅 GUI；测试在 Qt Test 主线程调用。

**Required tests:**
- 默认 0 行。
- 两条 Detection：`rowCount==2`，`label`/`box`/`confidence` 与输入一致。
- 再 `setDetections` 一条：`rowCount==1`。
- 空列表清空。
- Track：`trackId` 与 `state` 正确；Tentative/Confirmed/Lost 可区分。
- `roleNames` 含上述键。

**Acceptance criteria:**
- 模型 cpp 不 include `RuleEngine` / 插件实现头。
- 现有管线测试仍绿。

**Dependencies:** 无。接口变更：是（app 层）。并发风险：否（约定 GUI）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(ui): add DetectionModel and TrackModel list models`

---

### P8-T05  EventModel + PerformanceModel

**Task ID:** P8-T05

**Task Name:** 事件增量模型与性能快照

**Goal:** 事件按 session 增量追加；性能字段可被 250ms 定时器写入。本任务不启 QTimer、不写 QML 页。

**Files likely affected:**
- `tests/CMakeLists.txt`（`tst_eventmodel`、`tst_performancemodel`）

**New files:**
- `models/EventModel.h` / `.cpp`
- `models/PerformanceModel.h` / `.cpp`
- `models/EventTypeFilterModel.h` / `.cpp`（`QSortFilterProxyModel`，按 `EventType` 过滤；`typeFilter` 为 -1 表示全部）
- `tests/EventModelTest.cpp`
- `tests/PerformanceModelTest.cpp`

**Interfaces affected:**

```cpp
class EventModel : public QAbstractListModel {
    void beginSession();  // start() 时调用；之后 ingest 视为新 session
    void ingest(const std::vector<VisionEvent>& snapshot,
                const std::vector<StoredEvent>& history = {});
    // history 仅 beginSession 后第一次用于预填 DB 行（rowId 角色）；之后只处理 snapshot。
};

class PerformanceModel : public QObject {
    Q_PROPERTY(double captureFps READ ...) // 与 PipelineStats 同名的一组 READ 属性
    void update(const PipelineStats& stats);
};
```

Event roles：`sessionEventId`, `rowId`, `wallUtcMs`, `type`, `ruleId`, `trackId`, `label`, `confidence`, `message`, `direction`, `countIn`, `countOut`, `occupancy`, `frameId`。

`ingest(snapshot)`：对当前 session，只 append `eventId` 大于本 session 已见最大 id 的行。`beginSession` 不清空已有行（历史保留），只重置「本 session 已见 max id」为 0。

`EventTypeFilterModel::setTypeFilter(int)`：-1 全部，否则 `VisionEvent.type` 的整值。

`PerformanceModel::update` 写属性，有变化才 `emit xxxChanged`。无 Timer。

**Implementation outline:**
- EventModel 用 `beginInsertRows` 追加，禁止每帧 reset。
- `wallUtcMs`：来自 `StoredEvent` 用其值；来自直播 `VisionEvent` 用 ingest 时刻的 `QDateTime::currentMSecsSinceEpoch()`（同一 snapshot 内共用一个墙钟，避免同帧多事件时钟乱跳）。
- Performance 属性覆盖 Phase 7 已有字段（含 `eventsEmitted` / `avgRuleLatencyMs` / 跟踪计数）。

**Risks:**
- 用 `eventId` 跨 start 去重会把新会话 id=1 当成重复而丢事件——必须 `beginSession`。
- 每帧 reset EventModel 会卡 UI。

**Required tests:**
- snapshot 两条 id 1,2 → 两行；再 ingest 含 1,2,3 → 只有第三行插入，`rowCount==3`。
- `beginSession` 后再 ingest id=1 → 再增一行（不与上一 session 的 id=1 去重）。
- 空 snapshot 不改 rowCount。
- filter：只显示 `Loitering` 时源有 3 类各一条 → proxy `rowCount==1`。
- Performance：默认全 0；`update` 写入 captureFps 等；相同值不重复 emit（可用 QSignalSpy）。

**Acceptance criteria:**
- 不 include 检测器实现。
- 不在本任务创建 QTimer。

**Dependencies:** T03 仅当 `ingest` 预填 `StoredEvent`（结构在 T03）；若 T03 未做，history 参数可先只在头文件用前向声明，但执行顺序已要求 T03 在前。接口变更：是。并发风险：否。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(ui): add EventModel ingest and PerformanceModel snapshot`

---

### P8-T06  会话推理设置与停机重建管线

**Task ID:** P8-T06

**Task Name:** Settings 用的管线重建 API

**Goal:** 停机时可改 backend / precision / device / confidence / NMS / tracking，并重建 `VisionPipeline`。运行中拒绝。不写 Settings QML。

**Files likely affected:**
- `plugin/DetectorCreateRequest.h`（增加 `confidenceThreshold`、`nmsThreshold`，默认 0.25 / 0.45）
- `plugins/yolo/YoloVisionPlugin.cpp`（写入 `ModelConfig`）
- `utilities/CameraManager.h` / `.cpp`
- `inference/InferenceSelection.h` 不强制改；会话结构放 `utilities/SessionSettings.h` 或 `CameraManager.h`
- `tests/CameraManagerTest.cpp`
- `tests/CMakeLists.txt`（`tst_pluginmodel`）
- Face/Motion 插件忽略新字段（已忽略 backend）

**New files:**
- `models/PluginModel.h` / `.cpp`
- `utilities/SessionSettings.h`（若未放进 CameraManager）
- `tests/PluginModelTest.cpp`

**Interfaces affected:**

```cpp
struct SessionSettings {
    InferenceSelection inference;          // 启动时 = inferenceSelectionFromEnv()
    float confidenceThreshold = 0.25F;
    float nmsThreshold = 0.45F;
    bool trackingEnabled = true;
};

class CameraManager {
    SessionSettings sessionSettings() const;
    // running → false，不改动。
    // 否则 stop（若曾运行）、assembleFromPlugins 用新设置、重新 apply 已存 RuleSpec（T07 才有规则；本任务无 spec 则空引擎）。
    bool applySessionSettings(const SessionSettings& settings);

    PluginManager 只读：供 PluginModel 刷新
};

class PluginModel : public QAbstractListModel {
    void setPlugins(const std::vector<PluginMetadata>&,
                    const std::vector<std::string>& errors);
    // roles: pluginId, name, version, description, modeLabel, capabilities
};
```

`assembleFromPlugins`：`DetectorCreateRequest` 填入 session 的 backend/precision/device/confidence/nms。`trackingEnabled==false` 时 tracker 传 `{}`，否则 `ByteTrackTracker`。仍注入 `make_unique<RuleEngine>()`。

**Implementation outline:**
- 构造时 `m_session = inferenceSelectionFromEnv()` + 默认阈值。
- 不写环境变量、不写 QSettings（语言仍由 LocaleController 持久化）。
- PluginModel 由 CameraManager 在 scan 后填充；Dummy 的 mode 空，modeLabel 空字符串。

**Risks:**
- 运行中 assemble 会与 jthread 竞态——必须先 `stop`，且 `apply` 在 running 时直接 false（调用方应先停；API 仍 double-check）。
- 重建失败导致 CameraManager 无 pipeline：assemble 保持现有行为（缺插件则缺模式），不要留下空 unique_ptr 却 `isRunning` true。
- 把 confidence 做成 YOLO 运行时 setter 会碰到推理线程——禁止，必须重建检测器。

**Required tests:**
- PluginModel：两条 metadata → 两行，id/name 正确。
- CameraManager：注入 Fake 管线 running 时 `applySessionSettings` false，mode 仍 Face。
- 停机后 `trackingEnabled=false` 再 start：不要求 GPU；断言 `stats().activeTracks==0` 且有检测仍能出帧（FakeDetector + FakeVideoSource 的现有测试风格）。
- `DetectorCreateRequest` 默认阈值与现在 YOLO 所用 0.25/0.45 一致；Yolo 插件测试仍绿。
- 现有 `tst_cameramanager` 仍绿。

**Acceptance criteria:**
- QML 仍零改（本任务）。
- 运行中无法切 TensorRT。

**Dependencies:** 无（规则重注入在 T07 接上；本任务空引擎即可）。接口变更：是。并发风险：是（必须停机）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(app): rebuild pipeline from session inference settings when stopped`

---

### P8-T07  RuleModel 与停机 applyRuleSpecs

**Task ID:** P8-T07

**Task Name:** 规则列表模型

**Goal:** QML 可增删改 `RuleSpec`；停机 apply 进 `RuleEngine`。运行中 apply 失败。不画叠加。

**Files likely affected:**
- `utilities/CameraManager.h` / `.cpp`（存 `vector<RuleSpec>`，`applyRuleSpecs`，`start()` 前同步到引擎）
- `analytics/RuleEngine`（已有 clear，T02）
- `tests/CMakeLists.txt`（`tst_rulemodel`）
- `tests/CameraManagerTest.cpp` 或新测试
- `docs/analytics/rule-engine.md`（生产可经 applyRuleSpecs 注入，仍无默认 ROI）

**New files:**
- `models/RuleModel.h` / `.cpp`
- `tests/RuleModelTest.cpp`

**Interfaces affected:**

```cpp
class RuleModel : public QAbstractListModel {
    bool addSpec(RuleSpec spec);     // 空 id / 重复 id / makeRule 会失败的几何 → false
    bool removeAt(int row);
    bool setEnabled(int row, bool enabled);
    bool setLoiterSeconds(int row, double seconds); // 非 Loitering → false
    std::vector<RuleSpec> specs() const;
    void replaceAll(std::vector<RuleSpec> specs);
};

class CameraManager {
    std::vector<RuleSpec> ruleSpecs() const;
    bool applyRuleSpecs(std::vector<RuleSpec> specs); // running → false
};
```

RuleModel roles：`ruleId`, `kind`, `enabled`, `vertexCount`, `loiterSeconds`, `ax`, `ay`, `bx`, `by`。

`CameraManager::start()`：在 `m_pipeline->start()` **之前**（pipeline start 会 reset 引擎状态但保留规则——因此必须在第一次 start 前 addRule）。若引擎里已有规则且 specs 未变，仍以 specs 为权威：`clear` + `makeRule` + `addRule` + `setEnabled`。T06 重建管线后必须再次从 `m_ruleSpecs` 注入。

id 生成不放在 CameraManager：RuleModel `addSpec` 时若 `ruleId` 空，则分配 `roi-N` / `line-N` / `loiter-N` / `count-N`（N 为该 kind 已有个数+1）。

**Implementation outline:**
- CameraManager 不在 GUI 以外的线程碰 RuleModel。
- apply 失败不修改引擎。成功则替换 `m_ruleSpecs`。
- 生产默认仍是空列表。

**Risks:**
- 在 `start()` 之后 `addRule` 会与 evaluate 竞态——只在 `!isRunning()` 路径调用。
- T06 重建若忘了重新 addRule，UI 有规格但引擎空。

**Required tests:**
- RuleModel：加一条合法 ROI，`rowCount==1`；重复 id false。
- 非法多边形 false。
- CameraManager 注入可 start 的 Fake 管线：running 时 `applyRuleSpecs` false；stop 后 true，再 start，用 Fake 轨迹不一定走真实几何——至少 `stats().enabledRules==1`（需 pipeline 已跑过一帧 `onRuled`）。若 Fake 无规则求值时机，可直接在 stop 状态 apply 后检查 `pipeline` 不可达则改为：apply 后 `start`+跑几帧+`recentEvents` 不必非空（空引擎几何可能无事件），但 `enabledRules` 在有规则时于第一帧 evaluate 后 >0。
- `start` 两次（中间 stop）：规则仍在，不崩溃。
- `makeRule` 失败的 spec 被 apply 跳过还是整批失败？**锁定整批失败、引擎保持 apply 前状态**（先在临时 vector 全部 makeRule 成功再 clear）。

**Acceptance criteria:**
- 运行中引擎规则集不变。
- 不改四条规则 cpp 算法。

**Dependencies:** T02, T06（重建后重注入；若 T06 尚未把 hook 留好，本任务在 `assembleFromPlugins` 末尾调用 `injectRules()`）。接口变更：是。并发风险：是。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(app): apply RuleSpec lists to RuleEngine only while stopped`

---

### P8-T08  EventWriter 接入 CameraManager

**Task ID:** P8-T08

**Task Name:** 后台线程持久化 EventLog 增量

**Goal:** 推理产生的新事件入 SQLite，GUI 不执行 SQL。不写 Events 页。

**Files likely affected:**
- `utilities/CameraManager.h` / `.cpp`（拥有 EventWriter；`notifyFrame` 增量 enqueue；`start` 调 EventModel 不在本任务——只写 DB）
- `CMakeLists.txt`（app 链 `visionlab_storage`）
- `tests/CMakeLists.txt`（`tst_eventwriter`，PATH 加上 `eventwriter`）
- `docs/analytics/rule-engine.md` 或新 `docs/storage/events.md`（路径、10000、writer 线程、打开失败降级）

**New files:**
- `storage/EventWriter.h` / `.cpp`
- `tests/EventWriterTest.cpp`

**Interfaces affected:**

```cpp
class EventWriter {
public:
    explicit EventWriter(std::unique_ptr<IEventRepository> repo,
                         std::size_t queueCapacity = 1024);
    bool start(const std::filesystem::path& dbPath); // open+jthread；失败 false
    void enqueue(VisionEvent event);  // DropOldest，不阻塞 SQL
    void requestQuery(EventQuery query, QObject* receiver, const char* member);
    // 或 std::function 经 QMetaObject::invokeMethod 投递到 receiver 所在线程
    void stop();  // close 队列、join、close repo
};
```

CameraManager：
- 构造后 `EventWriter::start(appData/events.sqlite)`；失败 qWarning，后续 enqueue 可 no-op。
- `notifyFrame`：`recentEvents()`，对 `pipeline_event_id` / session 已 enqueue 的最大 id 之后的事件 `enqueue`。每次 `VisionPipeline::start` 成功后重置「已持久化 max id」（与 EventModel 的 beginSession 同一时机；本任务在 CameraManager::start 里重置 writer 游标）。
- 析构 `stop` writer 再停 pipeline（或先停 pipeline 再 stop writer，避免新事件）。锁定：**先 `pipeline->stop()`，再 `writer.stop()`**，避免 join 后仍 enqueue。
- `queryEvents` 转给 writer，结果 Queued 回调用方。测试用 QObject 接收。

**Implementation outline:**
- writer 循环：`pop` 一条或一批（可一次 pop 多条直到 empty）`insert` + `prune(10000)`。
- `enqueue` 在 GUI；队列满丢最旧。
- `requestQuery` 把查询请求也放进队列（variant：事件或 QueryJob），保证与 insert 同线程。
- 不用 detached thread。

**Risks:**
- 先毁 repo 再 join → UAF。所有权：writer 拥有 repo，join 在析构最前。
- GUI 调 `query()` 直接打 QSqlDatabase 会线程亲和性失败。
- EventLog 256 而 GUI 卡住很久会丢未持久化事件——文档写明；本阶段不加大 EventLog。
- `start()` reset EventLog 后 id 从 1：必须重置持久化游标，否则新 id=1 被当成已写。

**Required tests:**
- EventWriter + 临时 db：enqueue 3 条，短等（`QTRY_COMPARE` count==3），query 3 行、`wall_utc_ms>0`。
- 队列 DropOldest：可测 BoundedQueue 行为已有；此处 enqueue 2000 条后 count<=1024+已刷盘，不崩。不要写死延迟毫秒阈值。
- `open` 失败：start false，enqueue 不崩。
- CameraManager + Fake 管线 + FakeRule：跑若干帧后临时 db `count>0`（可给 CameraManager 测专用 ctor 注入 pipeline + 可注入 writer 路径；若生产路径写 AppData，测试用 `QStandardPaths` 重定向或给 `EventWriter` 测试替身）。**锁定：CameraManager 测试用 `QTemporaryDir` + 测试钩子 `setEventDatabasePath` 或构造注入 `unique_ptr<EventWriter>`。** 不要在开发者家目录断言。
- 现有 cameramanager / pipeline 测试仍绿。

**Acceptance criteria:**
- `notifyFrame` / 任何 Q_INVOKABLE 里无 `QSqlQuery`。
- 不改 InferenceWorker 签名（事件仍只进 EventLog；持久化在 GUI 增量读取）。

**Dependencies:** T03。接口变更：是。并发风险：是。新第三方依赖：否（沿用 T03 的 Sql）。需测量：否。

**Recommended Git commit message:** `feat(app): persist EventLog increments on a writer jthread`

---

### P8-T09  四页导航壳

**Task ID:** P8-T09

**Task Name:** Main 壳与页面文件

**Goal:** `Main.qml` 只做窗口与导航；四个页面文件就位；现有启停/模式/画面迁到 Monitor。接线已有模型属性。不画 ROI、Events 页可先空 ListView。

**Files likely affected:**
- `views/Main.qml`（瘦身为壳）
- `views/TitleBar.qml`（全局启停）
- `views/I18n.qml`（导航与页标题文案）
- `views/CameraView.qml`（先不改 fillMode，留给 T10）
- `controllers/VisionController.h` / `.cpp`（暴露 models、`currentPage`、`start/stop`；连接 `camera.frameChanged` 填 Detection/Track/Event；250ms timer 填 Performance；`startCamera` 里 `eventModel.beginSession()`）
- `main.cpp`（不必堆一堆 context property；模型经 VisionController）
- `CMakeLists.txt` `QML_FILES` 增加页面
- `docs/ui/qml-boundary.md`（可在 T12 写完；本任务至少在 VisionController 头注释写：QML 禁止碰 pipeline）

**New files:**
- `views/SideNav.qml`
- `views/MonitorPage.qml`
- `views/EventsPage.qml`（占位 ListView 绑 `eventFilterModel`）
- `views/PerformancePage.qml`（占位 Text 绑 PerformanceModel 若干属性）
- `views/SettingsPage.qml`（占位，真正控件 T12）

**Interfaces affected:**

```cpp
class VisionController {
    Q_PROPERTY(int currentPage READ ... WRITE ... NOTIFY ...) // 0..3
    Q_PROPERTY(DetectionModel* detectionModel READ ...)
    Q_PROPERTY(TrackModel* trackModel READ ...)
    Q_PROPERTY(EventModel* eventModel READ ...)
    Q_PROPERTY(EventTypeFilterModel* eventFilterModel READ ...)
    Q_PROPERTY(PerformanceModel* performanceModel READ ...)
    Q_PROPERTY(PluginModel* pluginModel READ ...)
    Q_PROPERTY(RuleModel* ruleModel READ ...)
    Q_PROPERTY(int frameWidth READ ...)
    Q_PROPERTY(int frameHeight READ ...)
};
```

模型对象 parent 为 VisionController，QML 不要 `new`。

**Implementation outline:**
- 左侧 `Repeater` 四个入口，选中色沿用现有 `#74D4FF` / 标题栏 `#0F172B`。
- `StackLayout` 四页。`Loader` 非必须（四页都轻）；不要 `Qt.createComponent(url)`。
- 把 Main 里模式 Repeater 移到 MonitorPage。
- TitleBar：语言左侧加启停，避免子页找不到停摄像头。Main 原 controlsBar 可删除或缩成 Monitor 的模式条。
- `frameChanged`：`setDetections` / `setTracks` / `eventModel.ingest(camera.recentEvents())`。需 `CameraManager::recentEvents()` 包装 `m_pipeline->recentEvents()`（若尚未暴露则本任务加）。
- Timer 250ms：`performanceModel.update(camera.statsSnapshot())`；`running==false` 仍可更新（全 0）。
- 不在本任务改 CameraView fillMode。

**Risks:**
- 每帧 ingest 全量 snapshot 若 EventModel 实现错会 O(n²)；依赖 T05 只 append。
- 把业务判断写进 QML `onClicked`（例如 makeRule）——禁止，只调 Q_INVOKABLE。
- `QtQuick.Window` 在 Qt 6 已不必要；现有 Main 有该 import，可删若无 `Window` 类型依赖——Main 根是 `Window`，应 `import QtQuick.Controls` + 根 `ApplicationWindow` 或保持 `Window`（QtQuick 已含）。不要大改样式系统。

**Required tests:**
- `VisionController` 现有无单测：给 `tst_visioncontroller` 或扩展 cameramanager——用 Fake 管线：start 后 `QTRY_VERIFY(detectionModel.rowCount()>=0)`，ingest 不崩。
- 现有 locale / cameramanager 仍绿。
- 无 qmltestrunner 要求；本任务以编译 + C++ 接线测试为准。

**Acceptance criteria:**
- 应用能启动；四页可切换；Monitor 仍能开摄像头（人工冒烟，不写假 FPS）。
- QML 不 import 任何 analytics/pipeline C++ 类型。

**Dependencies:** T04, T05, T06（PluginModel）, T07（RuleModel）。T08 不强制（无 DB 时 EventModel 仍可直播）。接口变更：是。并发风险：是（frameChanged 必须 GUI）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(ui): add four-page shell and bind list models to VisionController`

---

### P8-T10  Monitor：letterbox、规则叠加与绘制

**Task ID:** P8-T10

**Task Name:** ROI / 越线绘制

**Goal:** 停机时在画面上点出多边形或线段，写入 RuleModel；运行中只读叠加。配置经 T07 apply + start 进入 RuleEngine。

**Files likely affected:**
- `views/CameraView.qml`（`PreserveAspectFit`；叠加层）
- `views/MonitorPage.qml`（工具：ROI / 线 / 逗留 / 计数 / 选择）
- `views/I18n.qml`
- `controllers/VisionController.h` / `.cpp`（`Q_INVOKABLE` 映射与 commit 绘制）
- `rendering/Letterbox.h`（T01）
- `tests/LetterboxTest.cpp` 已有；可加 `VisionController` 映射测试或纯函数测试足够
- `docs/ui/qml-boundary.md`（帧坐标 vs item 坐标、letterbox）

**New files:**
- `views/RuleOverlay.qml`（根据 RuleModel 画已提交几何 + 当前草稿）

**Interfaces affected:**

```cpp
// VisionController
Q_INVOKABLE QPointF itemToFrame(qreal x, qreal y, qreal itemW, qreal itemH) const;
Q_INVOKABLE QPointF frameToItem(qreal fx, qreal fy, qreal itemW, qreal itemH) const;
Q_INVOKABLE bool itemPointInVideo(qreal x, qreal y, qreal itemW, qreal itemH) const;
enum DrawTool { None, Roi, Line, Loiter, Count }; // Q_ENUM
Q_INVOKABLE void beginDraw(DrawTool tool);      // running → 忽略
Q_INVOKABLE void addDrawPoint(qreal itemX, qreal itemY, qreal itemW, qreal itemH);
Q_INVOKABLE void finishDraw();  // 多边形 <3 或线段不足两点 → 丢弃草稿
Q_INVOKABLE void cancelDraw();
Q_INVOKABLE void commitRulesToEngine(); // 转 CameraManager::applyRuleSpecs(ruleModel.specs())
```

鼠标：内容区外点击忽略。多边形：单击加点，双击或 Enter `finishDraw`。线段：两点自动 finish。Escape `cancelDraw`。

运行中：`beginDraw` no-op；叠加仍显示已 apply 的 specs（RuleModel 即会话权威；未 start 前的 specs 也应画，便于停机编辑）。

**Implementation outline:**
- CameraView 根上叠 `RuleOverlay`，`anchors.fill` 与 Image 同一 item 尺寸（含黑边），绘制时用 `frameToItem`。
- 不修改 DetectionRenderer / TrackRenderer。
- `commitRulesToEngine` 供「应用规则」按钮；`startCamera` 内也调用一次 apply（T07 已要求 start 前注入）。避免重复定义。
- 有向线段 UI：从 A 画到 B，箭头只表示 A→B，不新造几何语义。

**Risks:**
- Image 的 margins（现有 `anchors.margins: 10`）必须计入映射的 item 尺寸——**锁定：映射相对显示 Image 的 item，不是外层圆角容器**。Letterbox 的 itemW/H 传 Image 的 width/height。
- 运行中修改 RuleModel 但不 apply 会让叠加与引擎不一致——运行中禁用 add/remove（QML `enabled: !VisionController.running`）。
- 把点存成 item 坐标：窗口一 resize 全错。必须存帧像素进 RuleSpec。

**Required tests:**
- C++：VisionController 在 frameWidth/Height=100、item 200×100 时，(50,0)→(0,0)（可构造 controller+Fake 帧尺寸属性；若太重则测 `computeLetterbox` + 一层 Q_INVOKABLE 转发的辅助函数）。
- RuleModel 经 `finishDraw` 等价路径：用 C++ 调 `addSpec` 已在 T07 覆盖；本任务至少保证 `itemToFrame` 拒绝黑边。
- 现有 renderer 测试仍绿。

**Acceptance criteria:**
- 停机画一个矩形 ROI（四点）+ start，引擎 `enabledRules>=1`（可用 CameraManager stats 在测试里走 addSpec 而非真鼠标）。
- Crop 不再使用。

**Dependencies:** T01, T07, T09。接口变更：是。并发风险：否（停机绘制）。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(ui): draw ROI and line rules in frame coordinates on Monitor`

---

### P8-T11  Events 页

**Task ID:** P8-T11

**Task Name:** 事件列表与类型过滤

**Goal:** 展示会话内（及启动时 DB 历史）事件；可按类型过滤。不在 GUI 做 SQL。

**Files likely affected:**
- `views/EventsPage.qml`
- `views/I18n.qml`
- `controllers/VisionController.cpp`（启动完成后 `requestQuery` 预填 `ingest({}, history)` 一次）
- `utilities/CameraManager.h`（把 T08 的 query 暴露给 controller）

**New files:** 无（页已在 T09 占位）

**Interfaces affected:**
- `VisionController::eventTypeFilter` int 属性，写入 `EventTypeFilterModel`。
- 启动：writer query `limit=500` 无 type 过滤 → `EventModel::ingest({}, history)` 在 `beginSession` 之前或之后？**锁定：构造/首帧前 `ingest` history（rowId 填充），然后每次 `startCamera` 只 `beginSession`，不清除 history 行。**

**Implementation outline:**
- ListView delegate：墙钟（`wallUtcMs` → 本地时间字符串在 QML `Qt.formatDateTime` 或 C++ role `timeText`）。**锁定：C++ 增加 `timeText` role**，避免 QML 自己除 1000。
- 列：时间、类型、ruleId、trackId、label、message。
- 顶部 ComboBox：全部 / ROI / 越线 / 逗留 / 计数。
- 空状态文案。不显示 snapshot 图。

**Risks:**
- 在 EventsPage `Component.onCompleted` 里直接打开第二个 `QSqlDatabase`。
- 过滤时重置源模型丢掉直播 append。

**Required tests:**
- EventModel history 预填 + 之后直播 ingest 的 T05 已覆盖；本任务加：filter 属性切换 proxy rowCount。
- 不强制 GUI 点击测试。

**Acceptance criteria:**
- 事件页可滚动、可过滤；开摄像头产生规则事件后列表增长（需 T10 配置规则；无规则时只显示历史）。
- QML 无 SQL。

**Dependencies:** T05, T08, T09。接口变更：小（filter 属性 / timeText）。并发风险：query 回调必须 Queued。新第三方依赖：否。需测量：否。

**Recommended Git commit message:** `feat(ui): add Events page with type filter and wall-clock timestamps`

---

### P8-T12  Performance 页、Settings 页与边界文档

**Task ID:** P8-T12

**Task Name:** 性能面板与安全设置页

**Goal:** 250ms 遥测可视化；设置项停机 Apply 重建管线。写 QML/C++ 边界文档。

**Files likely affected:**
- `views/PerformancePage.qml`
- `views/SettingsPage.qml`
- `views/I18n.qml`
- `controllers/VisionController.cpp`（`applySettings` Q_INVOKABLE → `CameraManager::applySessionSettings`；失败原因字符串）
- `docs/ui/qml-boundary.md`
- `docs/analytics/rule-engine.md`（补 UI 停机 apply、叠加层不进 renderer）
- `tests/CameraManagerTest.cpp`（Settings 路径已在 T06；本任务补 apply 失败文案若有 C++ API）

**New files:**
- `docs/ui/qml-boundary.md`

**Interfaces affected:**

```cpp
Q_INVOKABLE bool applyUiSettings(int backend, int precision, int deviceId,
                                 float confidence, float nms, bool tracking);
Q_INVOKABLE QString lastSettingsError() const;
Q_INVOKABLE bool applyUiRules(); // 即 commitRulesToEngine
```

backend 整型与 `InferenceBackend` 枚举值一致。INT8 不出现在 ComboBox。TensorRT 在未编译 `VISIONLAB_HAS_TENSORRT` 时仍可显示，Apply 后创建失败 → false + error，**保持旧 pipeline**（T06 应已 stop+assemble；若 assemble 得到缺 YOLO 的 detectors，与现网缺插件行为一致。**锁定：applySessionSettings 在 assemble 前把旧 pipeline 移到局部 backup，新 assemble 若 `detectors.empty()` 且旧的非空则恢复 backup 并 false。** 若 T06 未做 backup，本任务补上。）

Performance 页：capture/inference FPS、P50/P95、E2E、queue depth、dropped、activeTracks、eventsEmitted、avgRuleLatencyMs、enabledRules。普通 Text/Grid，不上 Chart 库。不要 16ms 刷一次。

Settings 页：backend、precision、device、confidence、NMS、tracking 开关、插件只读列表、规则列表 enable + loiter 秒、Apply。运行中 Apply 按钮 disabled 或按下得到 error「先停止摄像头」。

**Implementation outline:**
- 文档必须写：模型线程、250ms、letterbox、停机改规则/设置、EventWriter、QML 禁止的 include、与 Phase 9 基准的边界（本页数字是实时快照不是 benchmark 文件）。
- 不改推理内核。

**Risks:**
- 运行中 Apply 把 pipeline 拆掉导致悬挂 jthread。
- Performance 页用 `NumberAnimation` 绑 FPS 造成多余绑定计算——用静态 Text。
- 文档写「保证 60FPS」等无测量承诺——禁止。

**Required tests:**
- `applySessionSettings` running → false（T06）；backup 恢复：若本任务新增 backup，测 assemble 失败恢复。
- PerformanceModel 已测；本任务无新算法测试。
- 全量默认 ctest 仍绿（无 GPU 项不得无故变红）。

**Acceptance criteria（对照 Phase 8 DoD）：**
- UI 不跑推理 / evaluate / SQL。
- Track 叠加仍在（C++ renderer）+ 规则几何在 QML。
- Events 页（T11）+ 本页性能与设置。
- ROI/线经 RuleSpec 到达 RuleEngine（停机 apply + start）。
- SQLite 能存能查（T03/T08/T11）。
- 无高频性能信号。

**Dependencies:** T05, T06, T07, T09（Events 页 T11 可并行于本任务，但推荐先 T11）。接口变更：是。并发风险：是（Apply 停机）。新第三方依赖：否。需测量：否（禁止编造 FPS）。

**Recommended Git commit message:** `feat(ui): add performance and settings pages with stopped-pipeline apply`

---

## 任务总览

| Task ID | 名称 | 依赖 | 接口变更 | 并发风险 | 新依赖 | 需测量 | 新测试 |
|---|---|---|---|---|---|---|---|
| P8-T01 | Letterbox 映射 | 无 | 是 | 否 | 否 | 否 | 是 |
| P8-T02 | RuleSpec / clear / makeRule | 无 | 是 | 否 | 否 | 否 | 是 |
| P8-T03 | SQLite EventRepository | 无 | 是 | 否 | Qt6::Sql | 否 | 是 |
| P8-T04 | DetectionModel / TrackModel | 无 | 是 | 否 | 否 | 否 | 是 |
| P8-T05 | EventModel / PerformanceModel | T03 | 是 | 否 | 否 | 否 | 是 |
| P8-T06 | 会话设置与管线重建 | 无 | 是 | 是 | 否 | 否 | 是 |
| P8-T07 | RuleModel 停机 apply | T02, T06 | 是 | 是 | 否 | 否 | 是 |
| P8-T08 | EventWriter | T03 | 是 | 是 | 否 | 否 | 是 |
| P8-T09 | 四页壳 + 模型接线 | T04–T07 | 是 | 是 | 否 | 否 | 是 |
| P8-T10 | Monitor 绘制 | T01, T07, T09 | 是 | 否 | 否 | 否 | 是 |
| P8-T11 | Events 页 | T05, T08, T09 | 小 | 是 | 否 | 否 | 是 |
| P8-T12 | Performance / Settings / 文档 | T05–T07, T09 | 是 | 是 | 否 | 否 | 是 |

## Phase 8 DoD 对照

| DoD | 任务 |
|---|---|
| UI remains responsive | T05 250ms；T08 SQL 不在 GUI；T04 每帧模型行数小 |
| Track overlays work | 现有 TrackRenderer；T10 不拆除 |
| Event page works | T05, T08, T11 |
| Performance telemetry works | T05, T09, T12 |
| Settings can select implemented options safely | T06, T12 |
| ROI/line configuration reaches RuleEngine | T02, T07, T10 |
| QML does not own core business logic | T09–T12；`docs/ui/qml-boundary.md` |
| SQLite stores/query events | T03, T08, T11 |
| No high-frequency unnecessary UI allocations | T05 增量事件；性能 250ms；不把 rgb 放进模型 |
| Tests cover model roles / SQLite | T03–T05, T08 |

指定 Task ID 之前不要写实现代码。

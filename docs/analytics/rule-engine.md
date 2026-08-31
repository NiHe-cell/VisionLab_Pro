# Rule engine (VisionLab Pro Phase 7)

This document is the implementation contract for `IRule`, `RuleEngine`,
and the four V1 rules. T03–T09 must follow it. Changing a decision means
editing this file first.

Rules operate on `Track` objects. They are **not** detector plugins and
must not be added to `IVisionPlugin` capabilities. `ByteTrackTracker`
must not contain business rules.

V1 does **not** draw ROI / crossing lines on the frame, expose a QML
event page, persist events to SQLite, hot-reload rule config, or run a
dedicated RuleWorker thread. Those belong to Phase 8 or later.

## Pipeline insertion

Rules run **inside `InferenceWorker`**, after `ITracker::update` and
before overlay render:

```
CaptureWorker
  → BoundedQueue<FramePacket>  (DropOldest)
InferenceWorker
  → IDetector::detect
  → ITracker::update
  → RuleEngine::evaluate
  → render
  → LatestResult<PresentedFrame>
```

Capture is already decoupled by the bounded queue. Rule evaluation is
cheap geometry plus small per-track maps; it does not read pixels and
does not need its own `jthread` or second frame queue.

`IRule::evaluate` receives `tracks` and
`RuleContext{frameId, timestamp, sourceId}` only. It must not take
`cv::Mat` / `FramePacket::image`. Lost tracks that are still in the
vector use the predicted box foot point.

`LatestResult<PresentedFrame>` is newest-wins. Sparse events must not
live only on the presented frame. Pipeline keeps a bounded `EventLog`
(capacity **256**, DropOldest, mutex) for tests and Phase 8 snapshots.
`PresentedFrame.events` is still **this frame's new events**.

## Inside test (locked)

An object is inside a ROI when the **foot point** of its box lies in
the polygon, **including the boundary**.

- Foot point: `(box.x + width/2, box.y + height)`. Empty box → `(0, 0)`.
- Polygon: `vector<cv::Point2f>` with at least 3 vertices. Rectangles
  become four vertices via `polygonFromRect`.
- `cv::pointPolygonTest(..., measureDist=false) >= 0` means inside.
- Fewer than 3 vertices → not inside (evaluate returns no events).

Do not use bounding-box center or overlap ratio in V1.

## Deduplication (locked)

No millisecond cooldown. Each rule emits **transition or threshold**
events only:

- ROI intrusion: `outside → inside` only. Stay and exit emit nothing.
- Line crossing: one event per completed side change.
- Loitering: one event per stay when elapsed time reaches the threshold.
- Counting: `countIn++` only when the track is not already in the IN set.

Tracks missing from this frame's vector have left the scene. Drop their
per-rule state. Do **not** synthesize Exit / OUT events for disappearance.

## Directed crossing (locked)

Directed segment A→B. Side of a point is the sign of the 2D cross
product AB × AP: **+1 left, −1 right, 0 collinear** (absolute value
below `1e-6` counts as 0). Zero-length AB is side 0.

A completed crossing requires:

1. Previous side and current side are opposite and both non-zero.
2. Segment `previous → current` intersects `A → B` (endpoint touch
   counts as intersection; zero-length segments do not).

Then:

- `CrossingDirection::Forward`: + → −, `message = "A→B"`
- `CrossingDirection::Reverse`: − → +, `message = "B→A"`

Stopping on the line (current side 0) is not a crossing.

## State machines

### RoiIntrusionRule (`name() == "roi-intrusion"`)

1. Invalid polygon (`size < 3`): return `{}`.
2. Class filter: non-empty `classIds` skips other classes (treat as
   outside and clear that id's inside bit).
3. `inside = pointInPolygon(footPoint(box), polygon)`.
4. inside and this `trackId` was not inside last frame → one event,
   `message == "intrusion"`.
5. Still inside: no event. Outside: no Exit event.
6. Missing `trackId`: drop inside bit (reappearance is a new enter).
7. `reset()` clears the inside set.

### LineCrossingRule (`name() == "line-crossing"`)

1. A == B: return `{}`.
2. Same class filter as ROI.
3. First observation of a `trackId`: store foot point, no event.
4. Later frames: `classifyCrossing(prev, curr, a, b)`; non-`None`
   emits `LineCrossing` with `direction` and `A→B` / `B→A`.
5. Missing `trackId`: drop last foot (do not invent a crossing).
6. `reset()` clears last-foot map.

Do **not** treat “box intersects the line” as a crossing.

### LoiteringRule (`name() == "loitering"`)

1. Invalid polygon or `loiterSeconds <= 0`: return `{}`.
2. Default `loiterSeconds = 5.0`. Time comes from
   `RuleContext.timestamp`, never from frame counts.
3. Inside and no enter time: record `context.timestamp`, `emitted=false`.
4. Inside, not yet emitted, elapsed >= threshold → one event,
   `message == "loitering"`, `emitted=true`.
5. Still inside after emit: no further event.
6. Outside, class mismatch, or missing from the list: clear that id.
7. Lost-but-still-listed with foot still inside: timer continues.
8. Leave then re-enter: new enter time, may emit again.
9. `reset()` clears all timers.

### CountingRule (`name() == "counting"`)

Uses the same `classifyCrossing` as line crossing (do not compose a
`LineCrossingRule` instance; do not share maps).

1. Forward and `trackId` **not** in the IN set: `countIn++`, insert,
   event `message == "IN"`.
2. Reverse and **in** the IN set: `countOut++`, erase, event
   `message == "OUT"`.
3. Forward while already in IN: ignore (no double IN).
4. Reverse while not in IN: ignore.
5. Missing `trackId`: erase from IN set, occupancy drops, **no** OUT
   and no `countOut++`.
6. Occupancy is the IN-set size. Events copy current
   `countIn` / `countOut` / `occupancy`.
7. `reset()` zeros counters and maps.

A track may IN, OUT, then IN again (`countIn == 2`). “No double count”
means hysteresis via the IN set, not a lifetime ban.

## IRule / RuleEngine

`evaluate` / `reset` are not thread-safe. Only the inference thread
may call them during a running session. GUI `setMode` must not call
`reset()`; the worker resets on the next evaluate when the detection
mode changes. `VisionPipeline::start()` may `reset()` on the caller
thread **before** the inference `jthread` exists.

`eventId` is assigned by `RuleEngine`, starting at 1, never reused
until `reset()`. Rules return `eventId == 0`. Enable/disable lives on
the engine, not on `IRule`. Duplicate `ruleId` on `addRule` fails.

One `RuleEngine` instance is owned by one `VisionPipeline`, so
per-source state is the engine itself. V1 is a single camera.

A rule that throws `cv::Exception` contributes no events that frame;
other rules still run.

## Production wiring

`CameraManager::assembleFromPlugins` injects an **empty** `RuleEngine`
(no default ROI or line). There is no `VISIONLAB_RULES` environment
switch and no QML enable property (Phase 8).

An empty engine (or null `RuleEngine*`) leaves `PresentedFrame.events`
empty and new `PipelineStats` fields at 0 — same visible behavior as
Phase 6.

`IDetector` / `IVisionPlugin` / `ITracker` must not mention `IRule`.
The engine is owned by `VisionPipeline`, not created by plugins.

## Known V1 limits

- Foot-point inside can miss a large box that only overlaps the ROI
  with a corner.
- Greedy track ID swaps (Phase 6) can cause spurious enter/cross events.
- Configuration is constructor `*Config` values. No hot reload.
- No time-based cooldown on top of the state machines.

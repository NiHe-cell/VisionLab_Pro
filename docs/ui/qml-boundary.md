# QML / C++ boundary (Phase 8)

This document is the UI-side contract for VisionLab Pro. QML talks only to
`VisionController` and the list models it owns. Changing a decision means
editing this file first.

## Threading

- `VisionController`, `CameraManager`, and every `*Model` live on the GUI
  thread. QML bindings and `Q_INVOKABLE` calls stay on that thread.
- Capture and inference run on `VisionPipeline` `jthread`s. They must not
  touch QML objects, models, or widgets.
- `CameraManager` copies `PresentedFrame` into a `QImage` and emits
  `frameChanged` via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.
- `EventWriter::requestQuery` callbacks are also queued onto the GUI
  receiver. Do not open a second `QSqlDatabase` from QML.

## Visual shell

Chrome colors, type, and spacing come from the `Theme` QML singleton
(`views/Theme.qml`). Do not hard-code page backgrounds or title-bar paints.
Rule overlay strokes on the video stay kind-specific. Token dump:
`design-system/visionlab-pro/MASTER.md`.

## Telemetry cadence

`VisionController` starts one `QTimer` at **250 ms** and calls
`PerformanceModel::update(statsSnapshot())`. Do not emit per-frame
performance signals. Do not bind `NumberAnimation` to FPS. The Performance
page is a live snapshot, **not** a Phase 9 benchmark file and not a
throughput guarantee.

## Letterbox coordinates

`CameraView` uses `Image.PreserveAspectFit`. Rule clicks and overlays map
through `VisionController::itemToFrame` / `frameToItem` /
`itemPointInVideo`, which wrap `rendering/Letterbox.h`.

- `itemW` / `itemH` are the **Image item** size (the inner item that
  already has `anchors.margins: 10`), not the rounded outer container.
- `RuleSpec` vertices and segment endpoints are **frame pixels**. Storing
  item coordinates breaks on resize.
- Clicks on letterbox margins are ignored (`itemPointInVideo == false`).
- Detection and track boxes stay in the C++ renderer; they ride with the
  image and are not redrawn in QML.

## Stopped-pipeline edits

Change inference settings and rule geometry only while the camera is
stopped.

- `applySessionSettings` / `applyUiSettings` return false if running.
- `applyRuleSpecs` / `commitRulesToEngine` / `applyUiRules` return false
  if running.
- Monitor draw tools and Settings Apply are `enabled: !running`.
- `beginDraw` is a no-op while running. Overlays still show the current
  `RuleModel` specs.
- If assemble produces an empty detector map and the previous pipeline had
  detectors, `applySessionSettings` restores the backup pipeline and
  previous `SessionSettings`.

## EventWriter

The default `CameraManager` constructor opens
`AppData/events.sqlite` on a writer `jthread`. History is loaded once in
the `VisionController` constructor (`queryEvents`, `limit=500`) via
`EventModel::ingest({}, history)`. Each `startCamera` only calls
`beginSession()`; history rows stay. QML must not include `storage/` or
run SQL.

## QML must not include

QML and GUI delegates must not include or call:

- `pipeline/`, `analytics/` (`RuleEngine`, `IRule`, `evaluate`)
- detector / plugin create APIs
- `EventWriter` / `IEventRepository` / `QSqlDatabase`
- inference engine factories

Those stay behind `CameraManager` and `VisionController`.

## Phase 9

Numbers on the Performance page are the last 250 ms `PipelineStats`
snapshot. They are not benchmark artifacts and must not be copied into
reports as guaranteed FPS or latency.

# BYTE tracking (VisionLab Pro Phase 6)

This document is the implementation contract for `ITracker` and
`ByteTrackTracker`. T03–T08 must follow it. Changing a decision means
editing this file first.

This is an independent implementation of the BYTE association strategy
described by Zhang et al., ECCV 2022. It is **not** a copy of
`yolox/tracker/byte_tracker.py` or any third-party MOT repository.
No MOT sources are vendored. Kalman filtering uses OpenCV
`cv::KalmanFilter` already in the project.

V1 does **not** use the Hungarian algorithm, a dedicated Tracking
worker thread, detector plugins for tracking, or a QML TrackModel.

## Pipeline insertion

Tracking runs **inside `InferenceWorker`**, after `IDetector::detect`
and before overlay render:

```
CaptureWorker
  → BoundedQueue<FramePacket>  (DropOldest)
InferenceWorker
  → IDetector::detect
  → ITracker::update
  → render
  → LatestResult<PresentedFrame>
```

Capture is already decoupled by the bounded queue. BYTE association is
box matching plus a cheap Kalman step; it does not read pixels and does
not need its own `jthread` or second queue. A separate TrackingWorker
would add shutdown and lifetime complexity without helping backpressure.

`ITracker::update` receives `detections` and
`TrackUpdateContext{frameId, timestamp}` only. It must not take
`cv::Mat` / `FramePacket::image`.

Detectors and detector plugins remain unaware of tracking. Do not add
tracking to `IVisionPlugin` capabilities.

## BYTE association (locked)

Each `update`:

1. Compute `dt` from the previous `context.timestamp`. On the first
   call after construction or `reset()`, use `dt = 1/30` seconds.
   `dt <= 0` is treated as `1/30`.
2. `predict` every live track with the Kalman filter (SORT-style
   state `[cx, cy, a, h, vx, vy, va, vh]`, where `a = width / height`).
3. Split detections by confidence:
   - **high:** `confidence > highThresh`
   - **low:** `lowThresh < confidence <= highThresh`
   - **discard:** `confidence <= lowThresh`
4. **First association:** greedy IoU, high-score detections ↔ live
   tracks. Match only when `classId` is equal and IoU ≥ `matchIou`.
   Each detection and each track is used at most once. Pick the remaining
   pair with the largest IoU, repeat.
5. Unmatched **high-score** detections start new **Tentative** tracks
   (`createdTracks++`, next `trackId`).
6. **Second association:** greedy IoU, low-score detections ↔ tracks
   still unmatched after step 4 (occlusion recovery). New tracks are
   never created from low-score detections.
7. Tracks still unmatched: `timeSinceUpdate++`. On the transition into
   `Lost`, `lostTracks++`. If `timeSinceUpdate > maxLostFrames`, the
   track is **Removed** (`removedTracks++`) and dropped from storage.
8. Matched tracks: Kalman `update` with the detection box, `hits++`,
   `timeSinceUpdate = 0`, copy `label` / `confidence` / `box`. Promote
   to `Confirmed` when `hits >= minHits`.
9. `age` increments every frame a live track exists (including lost
   frames). New tracks start at `age = 1`, `hits = 1`.
10. Append a trajectory sample (centroid + box) for every live track.
    If `trajectory.size() > maxTrajectoryPoints`, `pop_front`.
11. Return only `Confirmed` and `Lost`. Do not return `Tentative` or
    `Removed`.
12. `activeTracks` is the size of the returned vector.
    `lastUpdateLatencyMs` is the wall-clock duration of that `update`.

`trackId` is a `uint64_t` that increases monotonically and is never
reused until `reset()`. `reset()` clears all tracks, restarts ids at 1,
and zeros `TrackerStats`. `VisionPipeline::start()` calls `reset()`
before launching worker `jthread`s so a later start does not reuse ids.

Default `TrackerConfig`:

| field | value |
|---|---|
| `highThresh` | 0.6 |
| `lowThresh` | 0.1 |
| `matchIou` | 0.5 |
| `maxLostFrames` | 30 |
| `minHits` | 3 |
| `maxTrajectoryPoints` | 30 |

## Threading

`update` / `reset` are not thread-safe. Only the inference thread may
call them during a running session. GUI `setMode` must not call
`reset()`; the worker resets on the next `update` when the detection
mode changes. `VisionPipeline::start()` may `reset()` on the caller
thread **before** the inference `jthread` exists.

## Production wiring

`CameraManager::assembleFromPlugins` injects `ByteTrackTracker`.
Tracking is always on in V1. There is no `VISIONLAB_TRACKING`
environment switch and no QML enable property (Phase 8).

Tracking is not a detector plugin capability. `IDetector` /
`IVisionPlugin` must not mention `ITracker`. The tracker is owned by
`VisionPipeline`, not created by plugins.

Motion mode uses the same tracker; motion-region ids may be unstable.

## Known V1 limits

- Greedy IoU can swap identities when two same-class boxes cross.
- Motion-region detections may receive unstable ids.
- Configuration is compile-time `TrackerConfig` defaults. No hot
  reload.

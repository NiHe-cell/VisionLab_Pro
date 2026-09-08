# Performance Page Overrides

> **PROJECT:** VisionLab Pro
> **Page Type:** Live telemetry snapshot
>
> Keep **MASTER dark tokens**. This is a data-dense dashboard, not a light BI theme.

---

## Layout

```
Title + LIVE / paused + last-update time
KPI card grid (2 × 3 or 3 × 4 depending on width)
Optional sparkline of last N snapshots (pause control required)
Hint: I18n.perfSnapshotHint
```

KPI set (existing model fields only — do not invent metrics):

- Capture FPS, Inference FPS, P50 ms, P95 ms, End-to-end ms
- Queue depth, Dropped frames, Active tracks, Events emitted
- Rule latency ms, Enabled rules

## Components

- KPI card: `--color-card`, 12 px padding, label `muted-foreground` 12 px, value monospace 21–28 px.
- Status: pair color with text (OK / warn / stale). Stale if timer not ticking.
- Sparkline: streaming area, current pulse `--color-accent`. Must have Pause. Under reduced motion show last static polyline.
- Cadence stays **250 ms** via `VisionController` timer. Do not bind `NumberAnimation` to FPS. Do not copy these numbers into reports as guaranteed FPS.

## Motion

- Value text updates in place; no count-up animation.
- Sparkline samples; pause freezes the buffer visually.

# VisionLab Pro Night Ops UI

Date: 2026-09-08  
Status: implemented, awaiting visual review  
Direction: Scheme A — Night Ops command center (user selected)

Token source: `design-system/visionlab-pro/MASTER.md`  
Page overrides: `design-system/visionlab-pro/pages/{monitor,events,performance,settings}.md`  
QML contract: `docs/ui/qml-boundary.md`

## 1. Goal

Restyle the existing Qt Quick shell so all four pages share one dark operations theme: video stays the hero on Monitor, telemetry is labeled as a live 250 ms snapshot, and chrome uses semantic tokens instead of mixed hardcoded hex.

## 2. Non-goals

- No new pages, plugins, backends, or detector modes.
- No C++ pipeline, inference, tracking, or storage changes.
- No QML calls into `pipeline/`, `analytics/`, plugin factories, or `EventWriter`.
- No Google Fonts / Fira files in this pass; map to system fonts.
- No light theme in this pass.
- No guaranteed FPS, latency, or benchmark numbers in the UI or reports.
- No macOS traffic-light window buttons.

## 3. Context (current UI)

Frameless 1200×900 window, custom title bar, 168 px dark rail, four `StackLayout` pages: Monitor, Events, Performance, Settings. Locale is zh/en via `I18n`. Camera start/stop and language live in the title bar. Draw tools and mode chips sit in a 96 px `#CAD5E2` slab above the video. Performance is a two-column label dump. Events is a `RowLayout` pretending to be a table. Settings is one ungrouped form. Hit targets are 15–28 px. Title-bar window buttons are 15 px colored circles.

Platform (locked): Windows desktop, rectangle window, desk viewing distance ~60 cm, mouse + keyboard, LTR, zh/en, GPU present (inference workstation).

## 4. Visual system

### 4.1 Color (MASTER tokens)

| Role | Hex | Use |
|------|-----|-----|
| Background | `#0F172A` | Window, page fill |
| Primary | `#1E293B` | Title bar, icon rail |
| Card | `#1B2336` | Command bar, KPI, table, settings groups |
| Muted | `#272F42` | Hover / selected wash |
| Foreground | `#F8FAFC` | Primary text |
| Muted foreground | `#94A3B8` | Labels, idle icons |
| Border | `#475569` | Hairlines |
| Accent | `#22C55E` | Start, Apply, LIVE, selected chip. On-accent `#0F172A` |
| Destructive | `#EF4444` | Stop, close, errors. On-destructive **white** (MASTER black fails 4.5:1) |
| Ring | `#FFFFFF` | Keyboard focus |

Status always pairs color with text or an icon. LIVE requires a timestamp from the 250 ms timer.

### 4.2 Type

- Body/heading: Segoe UI Variable / Segoe UI + Microsoft YaHei UI.
- Metrics: Cascadia Mono / Consolas.
- Scale (base 16, ratio 1.333): caption 12, body 16, h2 21, h1 28. At most four sizes on a screen.
- Body line-height 1.4–1.6. Heading 1.1–1.2.
- Prefer `font.pointSize` for body so OS DPI scaling applies.

### 4.3 Space and density

MASTER density 8: 2 / 4 / 8 / 12 / 16 / 24 / 32 px. Default control height 32 px. Title bar 48 px. Icon rail 64 px. Command bar 40–44 px.

### 4.4 Glass

Overlays on video only: fill `rgba(15, 23, 42, 0.55)`, blur 12–16 px via `MultiEffect`, 1 px `rgba(255,255,255,0.18)` border, radius 8–12 px. Do not blur the whole window. Opaque `--color-card` for tables and forms (contrast).

### 4.5 Icons

Phosphor outline, 20 px, regular stroke. Export SVG under `assets/icons/`. Rail: `VideoCamera`, `Warning`, `Pulse`, `Gear`. Tools/modes reuse outline glyphs. Decorative icons beside visible text are not accessibility names; icon-only buttons have `Accessibles.name`.

### 4.6 Motion

150–200 ms hover/focus. 250–300 ms page or camera fade. Animate `opacity` / `scale` only. `Theme.reducedMotion` (QSettings) → instant. Do not animate FPS digits. Do not use GSAP.

## 5. Shell

Keep `Qt.FramelessWindowHint`. `Main.qml` fill is `--color-background`, radius 12, clip.

**Title bar (48 px, `--color-primary`)**

Left: existing logo + `I18n.appTitle`.  
Right, in order: Start/Stop (height 32, Start = accent, Stop = destructive), language `EN`/`中` (ghost button), Windows min / max / close (32 px hit, 12 px glyph, muted fill; close hover destructive). Drag region is everything left of the control cluster. Double-click still maximizes.

**Icon rail (64 px)**

Four icon buttons, 40 px square, radius 8. Selected: muted fill + 3 px accent bar on the inner edge. Tooltip and Accessible name = existing nav strings. Below 1100 px width keep icons only (this is the default). Do not keep the current 168 px text-only rail.

**Focus**

Tab order: title controls → rail → page. Visible 2 px ring. Escape does not trap. ComboBox/popup dismiss on Escape.

## 6. Pages

`VisionController.currentPage` indices unchanged: 0 Monitor, 1 Events, 2 Performance, 3 Settings.

### 6.1 Monitor

Primary: live frame. Secondary: mode + draw tools. Tertiary: HUD snapshot.

Replace the 96 px gray header with a glass command bar:

- Left: Face / Object / Motion segmented control (existing `setMode` strings).
- Right: ROI / Line / Loiter / Count + Apply Rules.
- Tools and Apply stay `enabled: !running`.

`CameraView` keeps rounded stage, `PreserveAspectFit`, letterbox mapping, `RuleOverlay`. HUD chips overlay the image (FPS, P95, backend label if already exposed; do not add new C++ fields in this pass). Empty state: `I18n.cameraOff` on muted card.

### 6.2 Events

Title + type filter chips bound to `eventTypeFilter`. Table columns unchanged: time, type, rule, track, label, message. Header sticky. Row 36 px. Type chip uses `I18n.eventTypeLabel` plus color. Empty: `I18n.eventsEmpty`. Model remains `eventFilterModel`.

### 6.3 Performance

Title + LIVE/paused + last-update (timer tick). KPI cards for every existing `performanceModel` field. Optional sparkline of values already on the model; if the model has no history buffer, **skip the sparkline in this pass** rather than adding C++. Always show `I18n.perfSnapshotHint`. Do not present numbers as a benchmark.

### 6.4 Settings

One `ScrollView`, three cards: Inference, Plugins, Rules. Same bindings as today (`uiBackend`, sliders, `pluginModel`, `ruleModel`, `applyAll`). Apply disabled while running; show `lastSettingsError` in destructive color. Helper text when running: settings cannot apply until the camera stops (reuse existing behavior, make it visible).

## 7. Implementation shape

New QML singleton `Theme` (colors, space, type, `reducedMotion`). Optional `TypeScale.qml`. Replace hardcoded hex in `views/*.qml`. Shared controls: `IconNavButton`, `SegmentedControl`, `ToolChip`, `KpiCard`, `SettingsCard`. Keep `ControlButton` but restyle to Windows caption buttons.

CMake: register Theme singleton on the existing QML module; add SVG icons as resources. No new QML→C++ types unless Accessible names require it.

Files expected to change: `views/Main.qml`, `TitleBar.qml`, `SideNav.qml`, `MonitorPage.qml`, `CameraView.qml`, `EventsPage.qml`, `PerformancePage.qml`, `SettingsPage.qml`, `ControlButton.qml`, plus new theme/control QML and `assets/icons/*`. Tests that only click `VisionController` stay green; add/adjust QML or factory tests only if a selector breaks.

## 8. Accessibility and i18n

Contrast ≥ 4.5:1 for body on card/background (verified for `#F8FAFC` on `#0F172A` and `#94A3B8` on `#0F172A` for large/secondary; do not use muted-foreground for primary body). Focus visible. Keyboard reaches every control. Strings stay in `I18n.qml`; layouts tolerate 30–40% expansion. RTL not required this pass.

## 9. Risks

- `MultiEffect` blur on the command bar must not paint over the video item in a way that changes letterbox math (`itemW`/`itemH` stay the Image item).
- Dark ComboBox/Slider must still be usable; if Fusion light chrome leaks, set a Controls style overlay rather than per-control hacks.
- Icon-only rail needs tooltips + Accessible names or zh users lose labels.

## 10. Acceptance

- Four pages compile and run; start/stop, mode, draw tools, event filter, settings apply still work while respecting running-camera locks.
- No hardcoded `#CAD5E2` / mixed light slabs; tokens come from `Theme`.
- Title-bar Start is ≥ 32 px; window buttons are not 15 px dots.
- Performance copy still states live snapshot, not a benchmark.
- `docs/ui/qml-boundary.md` still holds.

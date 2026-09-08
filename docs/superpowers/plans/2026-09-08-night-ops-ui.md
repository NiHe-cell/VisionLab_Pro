# Night Ops UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restyle the existing Qt Quick shell to the Night Ops dark command-center theme without changing C++ pipeline behavior.

**Architecture:** A `Theme` QML singleton holds semantic colors, type, and spacing. Shared controls (`ToolChip`, `IconNavButton`, `KpiCard`, `SettingsCard`) consume Theme. Existing four pages and `VisionController` bindings stay; only views and CMake QML lists change.

**Tech Stack:** Qt 6.8 QML, Qt Quick Controls Basic (only in files that override `background`/`contentItem`), SVG icons, C++20 unchanged.

## Global Constraints

- C++20; QML must not include pipeline/analytics/plugin factories/EventWriter (`docs/ui/qml-boundary.md`).
- No new C++ properties except none this pass (HUD uses existing `performanceModel` + `uiBackend()`).
- Tokens from `design-system/visionlab-pro/MASTER.md`: bg `#0F172A`, card `#1B2336`, accent `#22C55E` on `#0F172A`, destructive `#EF4444` on white, focus ring `#FFFFFF`.
- System fonts only (Segoe UI Variable / Segoe UI / Microsoft YaHei UI; Cascadia Mono / Consolas for metrics).
- Hit targets ≥ 32 px. Icon rail 64 px. Title bar 48 px.
- Performance copy remains the 250 ms snapshot disclaimer. No invented FPS.
- Do not mix this work with the unrelated unstaged inference diffs.

---

### Task 1: Theme singleton, icons, shared controls, CMake

**Files:**
- Create: `views/Theme.qml`, `views/IconNavButton.qml`, `views/ToolChip.qml`, `views/KpiCard.qml`, `views/SettingsCard.qml`, `assets/icons/{video,warning,pulse,gear}.svg`
- Modify: `CMakeLists.txt` (QML_FILES, singleton flag, RESOURCES)
- Modify: `views/ControlButton.qml` (Windows caption, 32 px)
- Modify: `views/I18n.qml` (LIVE/paused, settings group titles, window button names, running lock hint)

- [ ] **Step 1:** Add Theme singleton and register it like `I18n.qml`.
- [ ] **Step 2:** Add four Phosphor-style outline SVGs and shared controls.
- [ ] **Step 3:** Restyle `ControlButton` to a 32×32 caption button with `glyph` and optional `destructive`.

### Task 2: Shell (Main, TitleBar, SideNav)

- [ ] **Step 1:** `Main.qml` background + Window palette from Theme; SideNav width 64.
- [ ] **Step 2:** TitleBar: 32 px Start/Stop (accent/destructive), ghost language, caption buttons; no traffic-light colors.
- [ ] **Step 3:** SideNav: four `IconNavButton`s with tooltips and Accessible names.

### Task 3: Four pages + CameraView HUD

- [ ] **Step 1:** Monitor command bar (ToolChip modes/tools) + CameraView HUD chips; remove `#CAD5E2` slab.
- [ ] **Step 2:** Events: filter chips + dark 36 px rows + type chips.
- [ ] **Step 3:** Performance: KPI grid + LIVE/time + existing hint; no sparkline.
- [ ] **Step 4:** Settings: three SettingsCards; Apply disabled while running with helper text.

### Task 4: Build, test, run

- [ ] **Step 1:** `cmake --build --preset dev-debug --target appVisionLab`
- [ ] **Step 2:** Run a short CPU ctest subset that does not need the GUI.
- [ ] **Step 3:** Launch `build/dev-debug/appVisionLab.exe` for the user.

Do not commit unless the user asks.

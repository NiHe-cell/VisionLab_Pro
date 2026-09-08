# Monitor Page Overrides

> **PROJECT:** VisionLab Pro
> **Page Type:** Live operations stage
>
> Keep **MASTER color, type, and density tokens**. This file only changes layout.

---

## Layout

Hero is the **camera stage**, not a marketing hero.

```
TitleBar (48px)
Icon rail (64px) | Command glass (40px)
                 | Video stage (fill) + HUD chips
                 | Optional status glass (bottom of video)
```

- Modes (Face / Object / Motion) are a segmented control on the command bar.
- Draw tools (ROI / Line / Loiter / Count) and Apply Rules sit on the same bar, right-aligned.
- Do not restore the old 96 px gray slab above the video.
- HUD (FPS, P95, backend) is overlay text + shape, not a second page of KPIs.
- Detection boxes stay in the C++ renderer; QML only draws `RuleOverlay`.

## Components

- Command bar: glass on `--color-card`, height 40–44 px, `--space-md` gaps.
- Tool chips: `--color-muted` idle, `--color-accent` selected, 32 px min height.
- Apply Rules: primary accent button, `enabled: !VisionController.running`.
- Empty camera: muted card, icon `VideoCamera`, copy from `I18n.cameraOff`.

## Motion

- Camera image opacity 250 ms when start/stop (already present). Skip if `Theme.reducedMotion`.
- No scroll-reveal. No GSAP.

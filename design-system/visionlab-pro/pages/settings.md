# Settings Page Overrides

> **PROJECT:** VisionLab Pro
> **Page Type:** Grouped form
>
> Keep **MASTER dark tokens**. Slightly looser density than Monitor (`--space-lg` / `--space-xl` between groups).

---

## Layout

Three cards in one scroll view:

1. **Inference** — backend, precision, device, confidence, NMS, tracking
2. **Plugins** — loaded plugin list + load errors
3. **Rules** — enable switches, loiter seconds for kind 2

Footer: Apply (primary) + error text (`lastSettingsError`).

## Components

- Labels 13 px `--color-muted-foreground`, controls 16 px body.
- ComboBox / SpinBox / Slider / Switch use the dark palette; no Fusion default light chrome.
- Apply: accent button, `enabled: !VisionController.running`. Show success or the existing error string after click. Do not silently no-op.
- Running camera: controls disabled with helper text, not just opacity.
- 30–40% string expansion for zh/en. No fixed-width label columns that clip.

## Motion

- No decorative stagger on form load.
- Slider value text updates immediately.

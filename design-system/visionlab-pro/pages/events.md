# Events Page Overrides

> **PROJECT:** VisionLab Pro
> **Page Type:** Operations log
>
> Keep **MASTER dark tokens**. Do not use a light or purple page palette.

---

## Layout

```
Page title + type filter chips (All / ROI / Line / Loiter / Count)
Dense table (fill)
Empty state centered when count == 0
```

- Sticky header row on `--color-card`.
- Row height 36 px. Hover `--color-muted`. Alternating rows optional at 4% white.
- Type is a chip (label + color). Color is not the only cue.
- Columns stay: Time, Type, Rule, Track, Label, Message.
- Horizontal scroll if the window is narrower than the table; do not crush columns below readable width.

## Components

- Filter chips: toggle buttons with `checked` / `accessible` name, not clickable `div`s.
- Empty: `Warning` icon (decorative, `aria-hidden`) + `I18n.eventsEmpty`.
- No carousel, testimonials, or marketing CTA.

## Motion

- Row highlight 150 ms fill only.
- Live ingest must not steal focus. Do not announce every new row; optional status “N new events” is a later task.

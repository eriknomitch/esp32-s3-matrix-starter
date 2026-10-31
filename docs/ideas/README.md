# Effect ideas

Five ideas for things to build on the Waveshare ESP32-S3-Matrix with this starter. Each file describes the idea and includes a prompt you can paste into a Claude Code session in this repo to build it.

| # | Idea | Touches | Try it without hardware |
|---|------|---------|-------------------------|
| 1 | [Falling sand / hourglass](01-falling-sand.md) | `lib/Effects` only | Yes |
| 2 | [A candle whose flame always rises](02-gravity-candle.md) | `lib/Effects` only | Yes |
| 3 | [Sloshing liquid](03-sloshing-liquid.md) | `lib/Effects` only | Yes |
| 4 | [Shake-to-roll dice](04-shake-dice.md) | `lib/Effects`, `Inputs`, IMU drivers, all three hosts | Yes, with a "shake" key |
| 5 | [Pass the marble between boards](05-marble-relay.md) | `lib/Effects`, new radio code in `src/` | Partly, with a loopback link in the preview |

Start with #1: it's the most impressive for the effort and fits the existing `Effect` interface as is. #4 adds shake detection, which #2 can reuse for its "blow out the candle" gesture.

## Using the prompts

Each prompt is self-contained and assumes the agent will read `CLAUDE.md`. The prompts share a few rules:

- Effects live in `lib/Effects/src/Effects.cpp` and are added to `kEffects`. Nothing in `lib/Effects` may include `Arduino.h` or other platform headers.
- Effects draw at full brightness. `src/display.cpp` applies the `cfg::kMaxBrightness` cap (40, on purpose). Don't raise it.
- Stateful effects restore their state in `reset()`, which runs on every effect switch.
- Rendering runs at about 30 fps (`cfg::kFrameMs` = 33 ms). Derive `dt` from `Inputs::ms` like `Marble` does.
- To check a change, run `just check`, which builds `matrix`, `sim`, `preview`, and the browser preview. Screenshot the browser preview headlessly as described in `CLAUDE.md`.

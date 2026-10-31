# 1. Falling sand / hourglass

About 30 colored grains fall toward whichever way is down, pile up, and slide off slopes. Flip the board and they pour to the other side. In hourglass mode, a diagonal "neck" lets one grain through at a time, so it doubles as a timer you flip over.

**Why it's the best one:** on a small grid, a tilt sensor feels most magical when it's moving lots of little objects at once, and sand does that better than anything.

**Fits the current code:** it only needs `tiltX`/`tiltY`, so it's a normal `Effect` and runs in the terminal and browser previews, where the physics is easiest to tune.

## Prompt

```text
Add two new effects to lib/Effects/src/Effects.cpp: "sand" and "hourglass". Read CLAUDE.md first.

Sand:
- A cellular-automaton sand simulation on the 8x8 grid with about 30 grains, each with
  its own color (a few related hues, e.g. warm sand tones with some variation, or a
  rainbow; pick what looks best on the board).
- Gravity comes from Inputs::tiltX/tiltY (screen space: +tiltX = right edge down,
  +tiltY = bottom edge down). Quantize the gravity vector to one of 8 directions
  (including diagonals). Each step, a grain moves one cell in that direction if the
  cell is empty, otherwise tries the two neighboring directions (randomly ordered so
  piles stay symmetric), otherwise stays.
- Step rate should scale with how steep the tilt is: nearly flat means grains barely
  move, steep means they pour. Below a small dead zone (board lying flat), don't move
  at all, so noise from the IMU doesn't make the pile jitter.
- Process grains in an order that runs against gravity (start from the cells nearest
  the "down" edge), so a falling column doesn't move as a solid block in one step.
  Keep updates deterministic per frame except for the left/right tie-break.
- Use dt from Inputs::ms like Marble does, and cap it (e.g. 100 ms) so a long stall
  doesn't teleport grains.
- reset() reseeds the grains in a heap at the top of the screen.

Hourglass:
- Same grain physics, but with a fixed wall of cells along the anti-diagonal (top-right
  to bottom-left), leaving a one-cell gap in the middle as the neck. Draw the walls dim.
- Start with all grains in the upper-left triangle. Grains pass through the neck at
  most one per N ms (pick N so a full run takes about 60 s), regardless of tilt, so it
  works as a timer. It runs when the board is held so the neck points down (a corner
  down, like a real hourglass); flipping the board 180 degrees runs it the other way.
- When the last grain passes, flash the bottom pile briefly.

Constraints:
- No platform headers in lib/Effects (it also builds as plain C++17 for the preview and
  under Emscripten for the browser).
- Draw at full brightness; src/display.cpp applies the brightness cap. Don't change
  cfg::kMaxBrightness.
- Put the new effects first in kEffects (sand, hourglass, then the existing ones), and
  note the new order in the README if it lists effects.

Verify:
- Run `just check` (builds matrix, sim, preview, and the browser preview).
- Start `node web/serve.mjs` in the background and screenshot the browser preview
  headlessly with Playwright as described in CLAUDE.md: once level, then after several
  arrow presses in each direction, to confirm grains pile toward the low edge. Show me
  the screenshots.

Then commit and push.
```

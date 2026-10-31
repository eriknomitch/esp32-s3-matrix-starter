# 2. A candle whose flame always rises

A classic fire effect, but the heat rises opposite to gravity, using the tilt reading as "up". Tilt the board and the flame bends to stay upright, like a real candle. A sharp shake blows it out: a puff of smoke, then it relights.

**Why it's cool:** it's very little code, but people's brains read it as a real flame.

**Fits the current code:** the flame is a normal `Effect`. The shake gesture works best with the acceleration input from [idea 4](04-shake-dice.md); without it, the prompt detects a shake from sudden tilt changes.

## Prompt

```text
Add a "candle" effect to lib/Effects/src/Effects.cpp. Read CLAUDE.md first.

Flame:
- Use a heat-diffusion fire (the classic "Doom fire" / Fire2012 style): a small heat
  grid, a source that injects random heat each frame, heat that spreads and rises
  while cooling, and a palette mapping heat to color (black -> deep red -> orange ->
  yellow -> near-white at the hottest).
- "Up" is opposite to gravity. Gravity in screen space is (Inputs::tiltX,
  Inputs::tiltY) (+tiltX = right edge down, +tiltY = bottom edge down). The heat
  source sits at the "down" edge and heat advects toward "up". The flame must look
  good in any direction, including diagonals, not just the four edges: do the
  advection in floating point (sample the heat grid at the position one step "down",
  with bilinear interpolation) rather than snapping to 4 directions.
- Smooth the gravity direction over a few frames so IMU noise doesn't make the flame
  flicker sideways. When the board lies nearly flat (small in-plane gravity), fall
  back to "up" = toward the top edge of the screen so it still looks like a candle on
  a table.
- Keep the source narrow (a wick about 2-3 cells wide in the middle of the down edge)
  so it reads as a candle flame, not a wall of fire.
- Use dt from Inputs::ms like Marble does, capped at about 100 ms, so the flame's speed
  doesn't depend on frame rate.

Blow-out gesture:
- If Inputs has an acceleration-magnitude or z field (added by a separate "shake to
  roll dice" change), detect a shake from that. Otherwise detect it from the tilt
  input: a change in (tiltX, tiltY) faster than a threshold over a couple of frames.
  Tune the threshold so ordinary tilting doesn't trigger it; in the browser preview a
  fast drag should.
- On a shake: stop injecting heat, show a short gray "smoke" wisp rising from the wick
  for about a second, stay dark for a moment, then relight with a small growing flame.

Constraints:
- No platform headers in lib/Effects. Draw at full brightness (the cap is in
  src/display.cpp). reset() restores all state (heat grid, smoothing, gesture state).
- Add it to kEffects after the existing effects.

Verify:
- Run `just check`.
- Start `node web/serve.mjs` in the background and take headless Playwright
  screenshots (see CLAUDE.md) level, tilted left, tilted right, and upside down, to
  confirm the flame always points away from gravity. Show me the screenshots.

Then commit and push.
```

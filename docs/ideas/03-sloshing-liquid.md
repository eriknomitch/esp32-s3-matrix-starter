# 3. Sloshing liquid

Half the matrix is water whose surface stays level as you rotate the board. It's modeled as a damped spring, so it overshoots and wobbles when you jerk the board, with a darker blue at the bottom and a bright line along the surface.

**Edge case:** lying flat, the in-plane gravity is near 0, so the water should ease to a calm, even state instead of flickering.

**Fits the current code:** it only needs `tiltX`/`tiltY`, so it's a normal `Effect` that runs in both previews.

## Prompt

```text
Add a "liquid" effect to lib/Effects/src/Effects.cpp: a container half full of water
whose surface stays perpendicular to real-world gravity and sloshes. Read CLAUDE.md
first.

Model:
- Gravity in screen space is (Inputs::tiltX, Inputs::tiltY) (+tiltX = right edge down,
  +tiltY = bottom edge down). The water fills the half-plane on the "down" side of a
  line perpendicular to the surface direction, through the center of the grid, so the
  volume stays about half.
- Don't snap the surface to the current gravity. Keep the surface angle (and optionally
  a small surface offset for sloshing) as a damped spring that's pulled toward
  gravity's direction, so a quick turn makes it overshoot and wobble for a second or
  two before settling. Wrap angles correctly so turning through 180 degrees doesn't
  spin the water the long way round.
- Add a standing wave along the surface: a small sinusoidal displacement whose
  amplitude is kicked by sudden changes in gravity and decays over time. That's what
  makes it look like liquid rather than a rotating line.
- When the in-plane gravity is small (board lying flat, |tilt| below about 0.15 g),
  the spring's pull weakens with it, so the water drifts gently instead of jumping
  around on IMU noise. Below a smaller threshold, ease the wave amplitude toward 0.
- Use dt from Inputs::ms like Marble does, capped at about 100 ms.

Rendering:
- Anti-alias the surface: compute each pixel's signed distance to the surface line
  (including the wave) and blend partial coverage, so the edge moves smoothly instead
  of stepping a whole pixel at a time.
- Color by depth: deep blue at the bottom, lighter cyan near the surface, and a thin
  bright (near-white) highlight right at the surface. The air side stays black, maybe
  with a very faint tint.

Constraints:
- No platform headers in lib/Effects. Draw at full brightness (the cap is in
  src/display.cpp). reset() restores all state.
- Add it to kEffects after the existing effects.

Verify:
- Run `just check`.
- Start `node web/serve.mjs` in the background and take headless Playwright
  screenshots (see CLAUDE.md): level, several arrow presses left, then a screenshot
  taken within ~300 ms of a sudden direction change to catch the wobble. Show me the
  screenshots.

Then commit and push.
```

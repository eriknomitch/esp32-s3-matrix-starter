# 4. Shake-to-roll dice

Shake the board to roll a die. It tumbles, slows down, and lands on a face. A die face fits the 8×8 grid neatly as a 3×3 layout of 2×2 pips. Variants: two dice (2d6) shown one after the other, or a Magic 8-Ball with a scrolling answer.

**Why it's cool:** it's actually useful at a game night, and it's the obvious "show a friend" demo.

**What changes:** `Inputs` only carries `tiltX`/`tiltY`, which can't tell a shake from a tilt. This adds the third accelerometer axis to `Inputs` so shakes can be detected from the total acceleration. [Idea 2](02-gravity-candle.md) can reuse it.

## Prompt

```text
Add shake detection to the input pipeline and a "dice" effect that rolls when you shake
the board. Read CLAUDE.md first; this touches every host.

1. Add the third axis to Inputs.
   - In lib/Effects/src/Effects.h, add `float accelZ;` to Inputs: acceleration along the
     axis out of the LED face, in g. Lying flat face up it reads about +1. Update the
     comment. With tiltX/tiltY it gives the full acceleration vector, so
     |a| = sqrt(x^2 + y^2 + z^2) is about 1 g at rest and spikes when shaken. Note that
     tiltX/tiltY are raw acceleration, so during a shake they can exceed 1.
   - src/imu.h: extend the API to return z (e.g. imuReadAccel(float& x, float& y,
     float& z)) and update both drivers. imu_qmi8658.cpp already reads az; map its sign
     so face-up is +1. That sign is unverified on hardware like the others, so add a
     comment next to it saying so. imu_mpu6050.cpp (the Wokwi build): same.
   - src/main.cpp: pass z through.
   - preview/preview.cpp: derive z from the faked tilt (sqrt(max(0, 1 - x^2 - y^2))),
     and add a key (e.g. `x`) that injects a decaying random jolt into all three axes
     for a few hundred ms, so you can shake in the terminal. Add it to the help line.
   - web: web_render() in web/web.cpp takes z too. In web/index.html, compute gravity's
     z in the board frame from the quaternion q (the same derivation as
     tiltFromPose(), third component, so face down it's -1) and pass it. Add a Shake
     button and a key (e.g. `x`) that inject a decaying random jolt the same way.
     Fast drags shouldn't count as shakes, since a real board being turned doesn't
     produce them either; only the explicit jolt should. Mention the key in the help.
   - Update CLAUDE.md where it describes Inputs, imu.h, the preview keys, and the web
     preview.

2. Add a "dice" effect to lib/Effects/src/Effects.cpp.
   - Shake detection: a shake is |a| deviating from 1 g by more than about 0.6 g, a few
     times within half a second. Tune it so ordinary handling doesn't trigger it.
   - While shaken: tumble, cycling quickly through random faces (never the same face
     twice in a row) with a slight jitter in position.
   - When shaking stops: keep rolling with the face changes slowing down over about a
     second, then land on the final face (from rand(); seed it once from the time of the
     first shake so each power-up doesn't repeat). Flash the face briefly on landing.
   - Draw pips as 2x2 blocks on a 3x3 grid, standard die layouts. Pick a pip color, or
     a different hue per value. Before the first roll, show a slowly pulsing 5 (or a
     "shake me" hint).
   - Orient the face so it's upright relative to gravity (the "down" edge from
     tiltX/tiltY, quantized to 4 directions, with hysteresis so it doesn't flip back
     and forth near 45 degrees).
   - reset() restores all state. Add it to kEffects after the existing effects.

Constraints:
- No platform headers in lib/Effects. Draw at full brightness (the cap is in
  src/display.cpp). Don't touch the pinned library versions.

Verify:
- Run `just check`.
- Start `node web/serve.mjs` in the background and drive the browser preview headlessly
  with Playwright (see CLAUDE.md): select the dice effect, press the shake key, and take
  screenshots mid-roll and after it settles. Show me the screenshots.
- Tell me which signs you couldn't verify without hardware.

Then commit and push.
```

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

PlatformIO/Arduino starter for the Waveshare ESP32-S3-Matrix (ESP32-S3, 8×8 WS2812 matrix on GPIO14, QMI8658 IMU on I2C 11/12, BOOT button on GPIO0).

## Commands

There are three PlatformIO environments (`platformio.ini`); `matrix` is the default.

```sh
pio run -e matrix -t upload   # build + flash the real board (QMI8658, USB-CDC serial)
pio device monitor            # 115200 baud, esp32_exception_decoder filter
pio run -e sim                # build for Wokwi (MPU6050 stand-in, UART0 serial); run via VS Code F1 → "Wokwi: Start Simulator"
pio run -e preview            # compile the effects natively for the desktop
pio run -e preview -t exec    # build + run the terminal preview (interactive: arrows/WASD tilt, space next, q quit)
node web/serve.mjs            # browser preview on http://localhost:8000, rebuilds + live-reloads on save (needs em++: brew install emscripten)
node web/serve.mjs build      # build the browser preview once (non-interactive compile check of lib/Effects under Emscripten)
```

The `justfile` wraps these (`just` lists the recipes): `flash`, `monitor`, `flash-monitor`, `sim`, `tui`, `web`, and `check`, which builds `matrix`, `sim`, `preview`, and `node web/serve.mjs build` in one go.

`scripts/cloud-setup.sh` is the setup script for the Claude cloud environment: it installs Emscripten, PlatformIO, and `just` (the last two via `uv tool`) and pre-warms the toolchains. It assumes root. The cloud environment's own setup script is a short bootstrap that calls it (the snippet is in `README.md`).

There are no tests and no linter. To check a change, compile the environments it affects. `-t exec` for the preview needs an interactive TTY and runs until `q`, so for a non-interactive check use `pio run -e preview` (build only). The ESP32 envs download the toolchain on first build, which takes several minutes.

If upload can't find the port: hold BOOT, tap RESET, release BOOT, upload again, then press RESET.

### Viewing the web preview headlessly

To check the browser preview without a display (e.g. in a cloud session), run `node web/serve.mjs` in the background, wait for `http://localhost:8000` to respond, then screenshot it with Playwright (`chromium.launch()`). Wait 1–2 s after load so the Wasm starts and the 80 ms `.device` transition settles. `#tilt` holds the tilt readout and `#name` the current effect. Each arrow key turns the board `KEY_STEP_DEG` (15°), so six presses in one direction leave it exactly edge-on and invisible; that's expected, not a rendering bug. A fresh browser context starts in the default Board view, since the view choice lives in `localStorage`.

## Architecture

The design centers on running the same effect code on four hosts:

- **`lib/Effects/`** is hardware-independent. `Frame` is an 8×8 RGB framebuffer (out-of-range `set` is ignored); `Effect` is the interface (`name`, `reset`, `render(Frame&, const Inputs&)`); `Inputs` carries `ms` plus tilt in g. Do not include `Arduino.h` or other platform headers here, because the `preview` env compiles it with plain C++17.
- **`src/`** is the firmware for `matrix` and `sim`. `main.cpp` runs a ~30 fps loop: debounce BOOT → read IMU → `effectAt(current).render(...)` → `displayShow`.
- **`preview/preview.cpp`** is the desktop host. The `preview` env uses `build_src_filter = -<*> +<../preview/>`, so nothing in `src/` is built for it. It draws with 24-bit ANSI color and fakes tilt from the keyboard.
- **`web/`** is the browser host, built outside PlatformIO. `web/serve.mjs` runs `em++` on `lib/Effects/src/Effects.cpp` + `web/web.cpp` into `web/build/effects.{mjs,wasm}` (gitignored), serves `web/`, and pushes reloads/compile errors to the page over server-sent events at `/events`. `web.cpp` exposes `extern "C"` `web_*` functions and copies each frame into a flat RGB buffer that `index.html` reads from `HEAPU8`. It must be linked with `em++`, not `emcc`, or the `Effect` vtables fail to link. `index.html` renders on the same 33 ms step as `cfg::kFrameMs` because `Marble` and `Sparkle` fade once per render, so trail length depends on frame rate. It also caps effect-time advance at 100 ms per frame so a backgrounded tab doesn't hand `Marble` a huge `dt`. There are two views, picked with the Board/Matrix toggle and stored in `localStorage`: `board` (default) is an estimate of the real board drawn from Waveshare's product photos, favoring looks over accuracy: front SVG (PCB, labelled pads, USB-C) under the LED canvas in a 250×268 layout, warm-gray LED packages (rim, lens, die) drawn by `drawBoardLeds()`, where a lit LED tints its package, burns to a near-white core, and blooms colored light onto its neighbors, a back SVG (USB-C receptacle, buttons, chips; pin names swapped sides), and a stack of `.edge` slices for the board's thickness; `matrix` is a plain panel of round LEDs with additive color. `LAYOUTS` in the script defines each view's LED geometry. The source of truth for tilt is the board's 3D orientation, a quaternion `q` in CSS's frame; keys and drag rotate it without limits (it can roll over), and `tiltFromPose()` derives the effects' `tiltX`/`tiltY` as gravity's x/y components in the board frame, like the IMU would report (so past 90° they shrink, and face down they're 0). The same `q` becomes the `matrix3d` on `.device`, which has front and back faces (`backface-visibility: hidden`). Drag listens on the untransformed `.stage` so an edge-on board can still be grabbed. `.stage` is a wide, canvas-like viewport (a `size` container, up to 1120 px wide, filling the height the text below leaves) and the device is `56cqmin` wide, small enough that its corners (0.733 board widths from the pivot) stay inside in every orientation, so a rotated board never covers the text below. Two HUD canvases sit in the viewport's top corners: `#pose` draws the board's X/Y/Z axes (`boardAxes()`, the same columns `matrix3d()` uses) around a miniature board, and `#gravity` is a bubble-level plot of `tiltX`/`tiltY` (hollow when face down). `#tilt` also shows how far the board is from level in degrees. Device tilt mode goes the other way and builds `q` from the measured tilt.

Responsibilities split across layers:

- **Coordinate conventions.** Effects use screen space: +tiltX means the right edge is down, +tiltY means the bottom edge is down (`imu.h`, `Effects.h`). Each IMU driver maps raw axes into this space. If tilt is wrong on hardware, fix the signs in `imu_qmi8658.cpp`, not in effects.
- **Brightness and rotation are applied only in `src/display.cpp`** (`cfg::kMaxBrightness` via NeoPixel `setBrightness`, `cfg::kRotation` via `indexFor`). Effects should draw at full brightness; the preview shows uncapped colors. LEDs are chained row-major from the top-left, which matches Wokwi's `wokwi-led-matrix` default.
- **IMU selection** is by preprocessor, not build filter: `imu_qmi8658.cpp` is wrapped in `#ifndef SIM` and `imu_mpu6050.cpp` in `#ifdef SIM`. Both implement `imu.h`. Only `matrix` pulls in SensorLib.
- **Effect registry.** To add an effect, subclass `Effect` in `lib/Effects/src/Effects.cpp` and add an instance to `kEffects` (order = button cycle order). Stateful effects (e.g. `Marble`, which derives `dt` from `Inputs::ms`) must restore their state in `reset()`, which both hosts call on effect switch along with `frame.clear()`.

## Hardware constraints

- `cfg::kMaxBrightness` is 40 on purpose: Waveshare warns that high brightness overheats and can damage the board. Don't raise it casually.
- Memory config (4 MB flash, 2 MB quad PSRAM, `qio_qspi`) is set in `platformio.ini` and mirrored in `diagram.json`. Keep the two in sync.
- `diagram.json` wires Wokwi's ESP32-S3-DevKitC-1 to the same GPIOs as the Matrix board. The DevKitC has no GPIO33/34.
- Versions are pinned to match Waveshare's tested setup: `espressif32@6.13.0` (Arduino-ESP32 2.0.17), Adafruit NeoPixel 1.12.0, SensorLib v0.1.6 (from a GitHub tag because it isn't in the PlatformIO registry). Don't bump them without a reason.
- Unverified-on-hardware settings: rotation (`cfg::kRotation`), tilt signs (`imu_qmi8658.cpp`), and color order (`NEO_GRB` vs `NEO_RGB` in `display.cpp`).

# ESP32-S3-Matrix Starter

Starter project for the [Waveshare ESP32-S3-Matrix](https://docs.waveshare.com/ESP32-S3-Matrix): an ESP32-S3 with an 8×8 RGB LED matrix and a QMI8658 motion sensor.

You can run the same code in three places:

| Where | Command | What runs |
|---|---|---|
| **Real board** | `pio run -e matrix -t upload` | Firmware with the QMI8658 sensor |
| **Wokwi simulator** | `pio run -e sim`, then F1 → **Wokwi: Start Simulator** | Firmware on a simulated ESP32-S3, with an MPU6050 standing in for the sensor |
| **Terminal preview** | `pio run -e preview -t exec` | Just the effects, drawn in your terminal. Starts instantly. |
| **Browser preview** | `node web/serve.mjs` | Just the effects, compiled to WebAssembly and served on http://localhost:8000 with live reload |

The demo has three effects: **marble** (tilt the board to roll a dot around), **rainbow**, and **sparkle**. Press **BOOT** to switch between them.

## Setup

1. Install [VS Code](https://code.visualstudio.com/).
2. Open this folder in VS Code and install the two recommended extensions when it asks:
   - **PlatformIO IDE**: builds and uploads the code. It downloads the ESP32 toolchain on the first build, which takes a few minutes.
   - **Wokwi Simulator**: press F1 → **Wokwi: Request a new License**. The license is free for hobby use.

## Workflow

### 1. Quick iteration: terminal preview

```sh
pio run -e preview -t exec
```

Use the arrow keys or WASD to tilt, space to switch effects, and q to quit. This only runs the code in `lib/Effects/`, so it's the fastest way to work on animations. Colors are shown at full brightness. The board caps them in `config.h`.

### 1b. Quick iteration: browser preview

```sh
brew install emscripten   # once
node web/serve.mjs
```

Open http://localhost:8000. The server compiles `lib/Effects/` to WebAssembly and rebuilds and reloads the page whenever you save a file in `lib/Effects/src/` or `web/`. Compile errors show up on the page. Drag the board to tilt it (double-click to level), or use the same keys as the terminal preview. On a phone or tablet, **Device tilt** uses the device's tilt sensor, but browsers only allow that on HTTPS or localhost. `node web/serve.mjs build` builds once without serving.

### 2. Full simulation: Wokwi

```sh
pio run -e sim
```

Then press F1 → **Wokwi: Start Simulator**. Wokwi reloads automatically each time you rebuild.

- **Tilt:** click the MPU6050 and drag the `accelX` and `accelY` sliders.
- **BOOT button:** click the blue button or press the space bar while the diagram has focus.
- **Serial output** appears in the Wokwi terminal.

The simulated board is Espressif's ESP32-S3-DevKitC-1, wired in `diagram.json` to the same GPIOs as the Matrix board. It's set up with the same memory: 4 MB flash and 2 MB PSRAM.

### 3. Real hardware

Plug in the board with USB-C, then run:

```sh
pio run -e matrix -t upload
pio device monitor
```

If upload can't find the port, enter download mode: hold **BOOT**, tap **RESET**, release **BOOT**, and upload again. After uploading this way, press RESET to start the program.

## Project layout

```
lib/Effects/        Hardware-independent effects + 8x8 framebuffer (runs everywhere)
src/main.cpp        Main loop: button, IMU, frame timing
src/config.h        Pins, brightness cap, rotation
src/display.cpp     Framebuffer -> WS2812 LEDs (Adafruit NeoPixel)
src/imu_qmi8658.cpp Real board's IMU (SensorLib)
src/imu_mpu6050.cpp Wokwi stand-in IMU (built only with -DSIM)
preview/            Terminal preview
web/                Browser preview (WebAssembly host, page, dev server)
diagram.json        Wokwi circuit
wokwi.toml          Points Wokwi at .pio/build/sim/firmware.bin
```

### Adding an effect

1. Subclass `Effect` in `lib/Effects/src/Effects.cpp`.
2. Add it to `kEffects`.

Effects only see a `Frame` plus `Inputs` (time and tilt). Because of that, the same effect works in both previews, in Wokwi, and on the board with no changes.

## Pins (from the board schematic)

| Function | GPIO |
|---|---|
| LED matrix data | 14 |
| IMU SDA / SCL | 11 / 12 |
| IMU INT1 / INT2 | 10 / 13 (not used here) |
| BOOT button | 0 |
| Header pins | 1–7, 33–40, TX (43), RX (44) |

Wokwi's DevKitC board doesn't have GPIO33 or GPIO34. Use other pins for anything you wire to those in the simulator.

## Checks on first real-hardware run

The simulator can't confirm these, so check them once on the real board:

- **Brightness:** `cfg::kMaxBrightness` is set to 40. Waveshare warns that high brightness overheats the board and can damage it. Raise it carefully.
- **Orientation:** if the image is sideways, change `cfg::kRotation` in `config.h`.
- **Tilt direction:** if the marble rolls uphill, flip the signs in `imu_qmi8658.cpp`. The current mapping follows Waveshare's motion demo.
- **Colours:** if they look swapped, change `NEO_GRB` to `NEO_RGB` in `display.cpp`. Waveshare's examples use both.

## Versions

- **Platform:** `espressif32@6.13.0`, which uses Arduino-ESP32 core 2.0.17. Waveshare tested their examples on 2.0.11.
- **Adafruit NeoPixel:** 1.12.0.
- **SensorLib:** v0.1.6, the same version Waveshare used. It's pulled from the GitHub tag because that version isn't in the PlatformIO registry.

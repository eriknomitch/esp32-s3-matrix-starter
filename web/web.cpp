// Browser preview: the same effects as the board, compiled to WebAssembly with
// Emscripten. index.html drives these functions and draws the pixels on a canvas.
// Build and serve with:  node web/serve.mjs
//
// stdout is the device's serial port: index.html shows it in its console, so
// the printf calls here mirror what src/main.cpp prints on the board.

#include <Effects.h>
#include <emscripten/emscripten.h>
#include <stdio.h>

#include "../src/config.h"  // plain constants (pins, brightness cap, frame step), safe to share

namespace {

Frame frame;
int current = 0;
uint8_t pixels[kWidth * kHeight * 3];  // row-major RGB, read by JS from wasm memory

void select(int i) {
  current = i % effectCount();
  effectAt(current).reset();
  frame.clear();
  printf("Effect: %s\n", effectAt(current).name());
}

}  // namespace

extern "C" {

// What setup() does on the board, minus the hardware: the boot banner, then the first effect.
EMSCRIPTEN_KEEPALIVE void web_setup() {
  printf("ESP32-S3-Matrix starter (%s)\n", "web preview");
  printf("IMU %s\n", "emulated from the board pose");
  printf("Press BOOT to change effect\n");
  select(0);
}

EMSCRIPTEN_KEEPALIVE void web_select(int i) { select(i); }

EMSCRIPTEN_KEEPALIVE void web_next() { select(current + 1); }

EMSCRIPTEN_KEEPALIVE int web_index() { return current; }

EMSCRIPTEN_KEEPALIVE const char* web_name() { return effectAt(current).name(); }

EMSCRIPTEN_KEEPALIVE int web_width() { return kWidth; }

EMSCRIPTEN_KEEPALIVE int web_height() { return kHeight; }

EMSCRIPTEN_KEEPALIVE const uint8_t* web_pixels() { return pixels; }

// Everything the page shows about the emulated device, as JSON: the firmware's
// config.h, the effect registry, and the compiler that built this module.
EMSCRIPTEN_KEEPALIVE const char* web_info() {
  static char json[1024];
  int n = snprintf(json, sizeof json,
                   "{\"width\":%d,\"height\":%d,\"ledPin\":%d,\"imuSda\":%d,\"imuScl\":%d,"
                   "\"buttonPin\":%d,\"maxBrightness\":%d,\"rotation\":%d,\"frameMs\":%u,"
                   "\"compiler\":\"%s\",\"effects\":[",
                   kWidth, kHeight, cfg::kLedPin, cfg::kImuSda, cfg::kImuScl, cfg::kButtonPin,
                   cfg::kMaxBrightness, cfg::kRotation, (unsigned)cfg::kFrameMs, __VERSION__);
  for (int i = 0; i < effectCount() && n < (int)sizeof json; i++) {
    n += snprintf(json + n, sizeof json - n, "%s\"%s\"", i ? "," : "", effectAt(i).name());
  }
  if (n < (int)sizeof json) snprintf(json + n, sizeof json - n, "]}");
  return json;
}

EMSCRIPTEN_KEEPALIVE void web_render(uint32_t ms, float tiltX, float tiltY) {
  effectAt(current).render(frame, Inputs{ms, tiltX, tiltY});
  uint8_t* p = pixels;
  for (int y = 0; y < kHeight; y++) {
    for (int x = 0; x < kWidth; x++) {
      Rgb c = frame.get(x, y);
      *p++ = c.r;
      *p++ = c.g;
      *p++ = c.b;
    }
  }
}

}  // extern "C"

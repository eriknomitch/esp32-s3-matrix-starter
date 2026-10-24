// Browser preview: the same effects as the board, compiled to WebAssembly with
// Emscripten. index.html drives these functions and draws the pixels on a canvas.
// Build and serve with:  node web/serve.mjs

#include <Effects.h>
#include <emscripten/emscripten.h>

namespace {

Frame frame;
int current = 0;
uint8_t pixels[kWidth * kHeight * 3];  // row-major RGB, read by JS from wasm memory

void select(int i) {
  current = i % effectCount();
  effectAt(current).reset();
  frame.clear();
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void web_select(int i) { select(i); }

EMSCRIPTEN_KEEPALIVE void web_next() { select(current + 1); }

EMSCRIPTEN_KEEPALIVE const char* web_name() { return effectAt(current).name(); }

EMSCRIPTEN_KEEPALIVE int web_width() { return kWidth; }

EMSCRIPTEN_KEEPALIVE int web_height() { return kHeight; }

EMSCRIPTEN_KEEPALIVE const uint8_t* web_pixels() { return pixels; }

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

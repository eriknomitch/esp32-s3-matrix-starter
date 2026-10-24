#pragma once

// Hardware-independent 8x8 framebuffer. No Arduino includes here, so the same
// code builds for the board, the Wokwi simulation and the desktop preview.

#include <stdint.h>
#include <string.h>

struct Rgb {
  uint8_t r, g, b;
};

constexpr int kWidth = 8;
constexpr int kHeight = 8;

class Frame {
 public:
  void clear() { memset(px_, 0, sizeof(px_)); }

  void fill(Rgb c) {
    for (auto& p : px_) p = c;
  }

  // Out-of-range writes are ignored, so effects don't need bounds checks.
  void set(int x, int y, Rgb c) {
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return;
    px_[y * kWidth + x] = c;
  }

  Rgb get(int x, int y) const { return px_[y * kWidth + x]; }

  // Multiply every pixel by f (0-255); used for fading trails.
  void fade(uint8_t f) {
    for (auto& p : px_) {
      p.r = p.r * f / 255;
      p.g = p.g * f / 255;
      p.b = p.b * f / 255;
    }
  }

 private:
  Rgb px_[kWidth * kHeight] = {};
};

// Hue 0-255 around the color wheel, full saturation.
Rgb hsv(uint8_t h, uint8_t s = 255, uint8_t v = 255);

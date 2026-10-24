#include "display.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"

namespace {

// The 64 LEDs are chained row by row, left to right, starting top-left
// (USB connector at the top). Wokwi's wokwi-led-matrix uses the same order by default.
Adafruit_NeoPixel strip(kWidth * kHeight, cfg::kLedPin, NEO_GRB + NEO_KHZ800);

int indexFor(int x, int y) {
  int rx = x, ry = y;
  switch (cfg::kRotation & 3) {
    case 1: rx = kWidth - 1 - y; ry = x; break;
    case 2: rx = kWidth - 1 - x; ry = kHeight - 1 - y; break;
    case 3: rx = y; ry = kHeight - 1 - x; break;
  }
  return ry * kWidth + rx;
}

}  // namespace

void displayBegin() {
  strip.begin();
  strip.setBrightness(cfg::kMaxBrightness);
  strip.clear();
  strip.show();
}

void displayShow(const Frame& f) {
  for (int y = 0; y < kHeight; y++) {
    for (int x = 0; x < kWidth; x++) {
      Rgb c = f.get(x, y);
      strip.setPixelColor(indexFor(x, y), c.r, c.g, c.b);
    }
  }
  strip.show();
}

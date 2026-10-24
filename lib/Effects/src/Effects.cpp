#include "Effects.h"

#include <math.h>
#include <stdlib.h>

Rgb hsv(uint8_t h, uint8_t s, uint8_t v) {
  uint8_t region = h / 43;
  uint8_t rem = (h - region * 43) * 6;
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * rem) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - rem)) >> 8))) >> 8;
  switch (region) {
    case 0: return {v, t, p};
    case 1: return {q, v, p};
    case 2: return {p, v, t};
    case 3: return {p, q, v};
    case 4: return {t, p, v};
    default: return {v, p, q};
  }
}

namespace {

// Diagonal rainbow that drifts over time.
class Rainbow : public Effect {
 public:
  const char* name() const override { return "rainbow"; }
  void render(Frame& f, const Inputs& in) override {
    uint8_t base = in.ms / 12;
    for (int y = 0; y < kHeight; y++)
      for (int x = 0; x < kWidth; x++) f.set(x, y, hsv(base + (x + y) * 16));
  }
};

// A marble that rolls toward whichever edge is tilted down, with a fading trail.
class Marble : public Effect {
 public:
  const char* name() const override { return "marble"; }
  void reset() override {
    x_ = y_ = 3.5f;
    vx_ = vy_ = 0;
    lastMs_ = 0;
  }
  void render(Frame& f, const Inputs& in) override {
    float dt = lastMs_ ? (in.ms - lastMs_) / 1000.0f : 0;
    lastMs_ = in.ms;

    constexpr float kGravity = 40.0f;  // pixels/s^2 per g
    constexpr float kFriction = 0.96f;
    constexpr float kBounce = -0.5f;
    vx_ = (vx_ + in.tiltX * kGravity * dt) * kFriction;
    vy_ = (vy_ + in.tiltY * kGravity * dt) * kFriction;
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    if (x_ < 0) { x_ = 0; vx_ *= kBounce; }
    if (x_ > kWidth - 1) { x_ = kWidth - 1; vx_ *= kBounce; }
    if (y_ < 0) { y_ = 0; vy_ *= kBounce; }
    if (y_ > kHeight - 1) { y_ = kHeight - 1; vy_ *= kBounce; }

    f.fade(140);
    f.set(lroundf(x_), lroundf(y_), hsv(in.ms / 20));
  }

 private:
  float x_ = 3.5f, y_ = 3.5f, vx_ = 0, vy_ = 0;
  uint32_t lastMs_ = 0;
};

// Random twinkles that fade out.
class Sparkle : public Effect {
 public:
  const char* name() const override { return "sparkle"; }
  void render(Frame& f, const Inputs&) override {
    f.fade(200);
    for (int i = 0; i < 2; i++) f.set(rand() % kWidth, rand() % kHeight, hsv(rand() % 256, 160));
  }
};

Rainbow rainbow;
Marble marble;
Sparkle sparkle;
Effect* const kEffects[] = {&marble, &rainbow, &sparkle};

}  // namespace

int effectCount() { return sizeof(kEffects) / sizeof(kEffects[0]); }

Effect& effectAt(int i) { return *kEffects[i]; }

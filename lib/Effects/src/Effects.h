#pragma once

#include "Frame.h"

// Everything an effect may react to. Tilt is in g, already mapped to screen
// space: +tiltX means the right edge is lower, +tiltY means the bottom edge is lower.
struct Inputs {
  uint32_t ms;
  float tiltX;
  float tiltY;
};

class Effect {
 public:
  virtual ~Effect() = default;
  virtual const char* name() const = 0;
  virtual void reset() {}
  virtual void render(Frame& f, const Inputs& in) = 0;
};

// Registry of built-in effects, in the order the button cycles through them.
int effectCount();
Effect& effectAt(int i);

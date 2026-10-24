#pragma once

#include <stdint.h>

// Pin assignments for the Waveshare ESP32-S3-Matrix (from the official schematic).
// diagram.json wires the Wokwi simulation to the same GPIOs.
namespace cfg {

constexpr int kLedPin = 14;     // WS2812B chain data in (level-shifted to 5 V on the board)
constexpr int kImuSda = 11;
constexpr int kImuScl = 12;
constexpr int kButtonPin = 0;   // BOOT button, usable as an input after boot

// Hard brightness cap (0-255). Waveshare warns that high brightness overheats
// and can damage the board; their demos use 40-60. Every frame goes through this.
constexpr uint8_t kMaxBrightness = 40;

// Rotate the image in 90-degree steps (0-3) if it appears sideways on the real board.
constexpr uint8_t kRotation = 0;

constexpr uint32_t kFrameMs = 33;  // ~30 fps

}  // namespace cfg

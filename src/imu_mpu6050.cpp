#ifdef SIM

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "imu.h"

// Minimal MPU6050 driver for the Wokwi stand-in. Wired to the same SDA/SCL
// pins as the real QMI8658, so the I2C setup is identical.

namespace {

constexpr uint8_t kAddr = 0x68;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegAccelXoutH = 0x3B;
constexpr float kLsbPerG = 16384.0f;  // default +/-2 g range

int16_t read16() {
  int16_t hi = Wire.read();
  return (hi << 8) | Wire.read();
}

}  // namespace

bool imuBegin() {
  Wire.begin(cfg::kImuSda, cfg::kImuScl);
  Wire.beginTransmission(kAddr);
  Wire.write(kRegPwrMgmt1);
  Wire.write(0);  // wake up
  return Wire.endTransmission() == 0;
}

bool imuReadTilt(float& tiltX, float& tiltY) {
  Wire.beginTransmission(kAddr);
  Wire.write(kRegAccelXoutH);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(kAddr, (uint8_t)4) != 4) return false;
  float ax = read16() / kLsbPerG;
  float ay = read16() / kLsbPerG;
  // The accelerometer reads the reaction to gravity, so a negative accelX means
  // the right edge is down. In Wokwi, click the MPU6050 and drag accelX/accelY.
  tiltX = -ax;
  tiltY = ay;
  return true;
}

#endif

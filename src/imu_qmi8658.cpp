#ifndef SIM

#include <Arduino.h>
#include <SensorQMI8658.hpp>
#include <Wire.h>

#include "config.h"
#include "imu.h"

namespace {
SensorQMI8658 qmi;
}

bool imuBegin() {
  Wire.begin(cfg::kImuSda, cfg::kImuScl);
  // SA0 is tied to GND on this board -> address 0x6B
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, cfg::kImuSda, cfg::kImuScl)) return false;
  qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_125Hz,
                          SensorQMI8658::LPF_MODE_0, true);
  qmi.enableAccelerometer();
  return true;
}

bool imuReadTilt(float& tiltX, float& tiltY) {
  if (!qmi.getDataReady()) return false;
  float ax, ay, az;
  if (!qmi.getAccelerometer(ax, ay, az)) return false;
  // Axis mapping follows Waveshare's Game demo (+ax moves the dot down, +ay moves it left).
  // If the marble rolls the wrong way on your board, flip the signs here.
  tiltX = -ay;
  tiltY = ax;
  return true;
}

#endif

#pragma once

// Tilt sensor abstraction. The real board has a QMI8658 (imu_qmi8658.cpp);
// Wokwi has no QMI8658, so the SIM build uses its MPU6050 (imu_mpu6050.cpp).

bool imuBegin();

// Tilt in g, mapped to screen space (+x = right edge down, +y = bottom edge down).
// Returns false if no new sample was available.
bool imuReadTilt(float& tiltX, float& tiltY);

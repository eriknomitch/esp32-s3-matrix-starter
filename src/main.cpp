#include <Arduino.h>
#include <Effects.h>

#include "config.h"
#include "display.h"
#include "imu.h"

namespace {

Frame frame;
int current = 0;
bool imuOk = false;
float tiltX = 0, tiltY = 0;

void selectEffect(int i) {
  current = i % effectCount();
  effectAt(current).reset();
  frame.clear();
  Serial.printf("Effect: %s\n", effectAt(current).name());
}

// Returns true once per press of the BOOT button (debounced).
bool buttonPressed() {
  static bool last = HIGH;
  static uint32_t changedAt = 0;
  bool now = digitalRead(cfg::kButtonPin);
  if (now != last && millis() - changedAt > 30) {
    changedAt = millis();
    last = now;
    return now == LOW;
  }
  return false;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(cfg::kButtonPin, INPUT_PULLUP);
  displayBegin();

  imuOk = imuBegin();
  Serial.printf("ESP32-S3-Matrix starter (%s)\n",
#ifdef SIM
                "Wokwi simulation"
#else
                "hardware"
#endif
  );
  Serial.printf("IMU %s, PSRAM %u bytes\n", imuOk ? "ok" : "NOT FOUND", ESP.getPsramSize());
  Serial.println("Press BOOT to change effect");
  selectEffect(0);
}

void loop() {
  static uint32_t nextFrame = 0;
  if (buttonPressed()) selectEffect(current + 1);

  uint32_t now = millis();
  if ((int32_t)(now - nextFrame) < 0) return;
  nextFrame = now + cfg::kFrameMs;

  if (imuOk) imuReadTilt(tiltX, tiltY);
  effectAt(current).render(frame, Inputs{now, tiltX, tiltY});
  displayShow(frame);
}

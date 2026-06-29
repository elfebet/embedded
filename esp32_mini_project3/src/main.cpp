#include <Arduino.h>
#include "AlarmBlink.h"
#include "PullDownButton.h"
#include "LaserDetector.h"

constexpr uint8_t PIN_LDR = 4;          // photoresistor
constexpr uint8_t PIN_ALARM_LED = 5;    // red led
constexpr uint8_t PIN_BUTTON = 6;       // button
constexpr uint8_t PIN_BUZZER = 7;       // piezo buzzer (sound)

PullDownButton button = PullDownButton(PIN_BUTTON);
AlarmBlink alarmBlink = AlarmBlink(PIN_ALARM_LED);
LaserDetector laserDetector = LaserDetector(PIN_LDR);

void setup() {
  Serial.begin(115200);

  alarmBlink.setup();
  button.setup();
}

void loop() {
  if (button.isPressed()) {
    if (laserDetector.isLaserDetected) {
      laserDetector.resetDetection();
    } else {
      laserDetector.toggleMaskingMode();
    }
  }

  laserDetector.detectLaser();
  alarmBlink.update(laserDetector.isLaserDetected);
}
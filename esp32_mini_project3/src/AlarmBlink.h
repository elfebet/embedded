#pragma once
#include <Arduino.h>

class AlarmBlink {
private:
    const uint8_t _ledPin;
    const uint8_t _buzzerPin; // if UINT8_MAX -> pin unused
    const int _blinkDelay;

    bool _isBlinkOn = false;
    unsigned long _lastTime = 0;

    void setBlinkOn(bool isOn);
public:
    AlarmBlink(uint8_t ledPin, uint8_t buzzerPin = UINT8_MAX, int blinkDelay = 500);
    void setup();
    void update(bool isBlinkEnabled);
};
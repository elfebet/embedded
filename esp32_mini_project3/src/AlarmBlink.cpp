#include "AlarmBlink.h"

AlarmBlink::AlarmBlink(uint8_t ledPin, uint8_t buzzerPin, int blinkDelay)
 : _ledPin(ledPin), _buzzerPin(buzzerPin), _blinkDelay(blinkDelay) {
}

void AlarmBlink::setup() {
    pinMode(_ledPin, OUTPUT);
    if (_buzzerPin != UINT8_MAX) {
        pinMode(_buzzerPin, OUTPUT);
    }
}

void AlarmBlink::setBlinkOn(bool isOn) {
    _isBlinkOn = isOn;
    uint8_t val = isOn ? HIGH : LOW;
    digitalWrite(_ledPin, val);
    if (_buzzerPin != UINT8_MAX) {
        digitalWrite(_buzzerPin, val);
    }
}

void AlarmBlink::update(bool isBlinkEnabled) {
    if (isBlinkEnabled) {
        unsigned long currentTime = millis();
        if (currentTime - _lastTime > _blinkDelay) {
            _lastTime = currentTime;
            setBlinkOn(!_isBlinkOn);
        }
    } else {
        setBlinkOn(false);
    }
}
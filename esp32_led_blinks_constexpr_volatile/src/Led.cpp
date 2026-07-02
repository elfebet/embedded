#include "Led.h"
#include <Arduino.h>

void Led::setup() {
    GPIO.enable |= LED_MASK; // Enable the GPIO pin for output
    // pinMode(LED_PIN, OUTPUT);
    setState(_state);
}

LedState Led::getState() const {
    return _state;
}

void Led::setState(LedState state) {
    _state = state;
    switch (_state) {
        case LedState::Off:
            GPIO.out &= ~LED_MASK;
            // digitalWrite(LED_PIN, LOW);
            break;
        case LedState::On:
            GPIO.out |= LED_MASK;
            // digitalWrite(LED_PIN, HIGH);
            break;
        case LedState::Blinking:
            _lastBlinkMs = 0;
            break;
    }
}

void Led::loop() {
    if (_state != LedState::Blinking) return;

    unsigned long now = millis();
    if (now - _lastBlinkMs >= LED_BLINKS_MS) {
        _lastBlinkMs = now;
        GPIO.out ^= LED_MASK;
        // _isOn = !_isOn;
        // digitalWrite(LED_PIN, _isOn ? HIGH : LOW);
    }
}

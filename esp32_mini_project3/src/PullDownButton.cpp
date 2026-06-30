#include "PullDownButton.h"

PullDownButton::PullDownButton(uint8_t pin, int debounceDelay)
    : _pin(pin), _debounceDelay(debounceDelay) {
}

void PullDownButton::setup() {
    pinMode(_pin, INPUT_PULLDOWN);
}

bool PullDownButton::isPressed() {
    int reading = digitalRead(_pin);
    if (reading != _lastState) {
        _lastDebounceTime = millis();
    }

    bool isPressed = false;
    if (millis() - _lastDebounceTime > _debounceDelay) {
        if (reading != _lastStableState) {
            _lastStableState = reading;
            if (_lastStableState == HIGH) {
                isPressed = true;
            }
        }
    }

    _lastState = reading;
    return isPressed;
}
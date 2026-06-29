#pragma once
#include <Arduino.h>

class PullDownButton {
private:
    const uint8_t _pin;
    const int _debounceDelay;
    uint8_t _lastState = LOW;
    uint8_t _lastStableState = LOW;
    unsigned long _lastDebounceTime = 0;
public:
    PullDownButton(uint8_t pin, int debounceDelay = 50);
    void setup();
    bool isPressed(); // call this function in loop()
};
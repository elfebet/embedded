#pragma once
#include "config.h"
#include <Arduino.h>

class Led {
private:
    LedState _state;
    bool _isOn = false;
    unsigned long _lastBlinkMs = 0;
public:
    explicit Led(LedState state = LedState::Off): _state(state) {}
    LedState getState() const;
    void setState(LedState state);
    void setup();
    void loop();
};
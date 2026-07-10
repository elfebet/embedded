#pragma once
#include "Arduino.h"

class ButtonBase {
    protected:
        uint8_t pin;
    public:
        ButtonBase(uint8_t pin): pin(pin) {}
        virtual ~ButtonBase() = default;
        virtual uint8_t buttonType() const = 0;
        virtual void setup() = 0;
        virtual void loop() = 0;
};
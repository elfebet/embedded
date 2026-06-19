#include <Arduino.h>

typedef void (*ButtonCallback)();

class Button {
private:
    uint8_t pin;
    uint32_t debounceDelay;
    uint32_t longPressDelay;

    bool lastState = HIGH;
    bool currentState = HIGH;
    uint32_t lastDebounceTime = 0;
    uint32_t pressedTime = 0;
    bool isPressHandled = true;
    bool isLongPressHandled = true;

    ButtonCallback shortPressCallback = nullptr;
    ButtonCallback longPressCallback = nullptr;

public:
    Button(uint8_t pin, uint32_t debounceDelay = 50, uint32_t longPressDelay = 1000) 
        : pin(pin), debounceDelay(debounceDelay), longPressDelay(longPressDelay) {}

    void setup() {
        pinMode(pin, INPUT_PULLUP);
    }

    void onShortPress(ButtonCallback callback) {
        shortPressCallback = callback;
    }

    void onLongPress(ButtonCallback callback) {
        longPressCallback = callback;
    }

    void update() {
        bool reading = digitalRead(pin);

        if (reading != lastState) {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > debounceDelay) {
            if (reading != currentState) {
                currentState = reading;

                if (currentState == LOW) {
                    pressedTime = millis();
                    isPressHandled = false;
                    isLongPressHandled = false;
                }
            }
        }

        if (currentState == LOW && !isLongPressHandled) {
            if ((millis() - pressedTime) >= longPressDelay) {
                Serial.println("Long Press Detected");
                if (longPressCallback) {
                    longPressCallback();
                }
                isLongPressHandled = true;
                isPressHandled = true; 
            }
        }

        if (currentState == HIGH && !isPressHandled) {
            Serial.println("Short Press Detected");
            if (shortPressCallback) {
                shortPressCallback();
            }
            isPressHandled = true;
        }

        lastState = reading;
    }
};

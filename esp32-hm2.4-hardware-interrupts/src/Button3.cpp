#include "ButtonBase.h"
#include <FunctionalInterrupt.h>

// Завдання 3: Debounce через перевірку рівня (state-based)
class Button3 : public ButtonBase {
    private:
        volatile bool isInterrupted = false;
        const unsigned long debounce = 50;
        unsigned long lastDebounceTime = 0;
        bool lastPressedState = false;
        bool isPressed = false;

        void IRAM_ATTR handleInterrupt() {
            isInterrupted = true;
        }

    public:
        Button3(uint8_t pin) : ButtonBase(pin) {
            Serial.println("Button3 init");
        }

        virtual ~Button3() {
            detachInterrupt(digitalPinToInterrupt(pin));
            Serial.println("Button3 deinit");
        }

        virtual uint8_t buttonType() const override { return 3; }

        virtual void setup() override {
            pinMode(pin, INPUT_PULLDOWN);
            attachInterrupt(digitalPinToInterrupt(pin), std::bind(&Button3::handleInterrupt, this), CHANGE);
        }

        virtual void loop() override {
            if (isInterrupted) {
                bool pressed = digitalRead(pin) == HIGH;
                if (pressed != lastPressedState) {
                    lastDebounceTime = millis();
                }

                if (millis() - lastDebounceTime > debounce && pressed != isPressed) {
                    isPressed = pressed;
                    if (isPressed) {
                        Serial.println("Button pressed");
                    }
                }

                lastPressedState = pressed;
            }
        }
};
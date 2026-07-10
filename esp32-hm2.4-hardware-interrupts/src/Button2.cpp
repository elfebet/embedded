#include "ButtonBase.h"
#include <FunctionalInterrupt.h>

// Завдання 2: Software debounce через таймер (time-based)
class Button2 : public ButtonBase {
    private:
        volatile bool isPressed = false;
        const unsigned long debounce = 50;
        unsigned long lastTime = 0;

        void IRAM_ATTR handleInterrupt() {
            isPressed = true;
        }

    public:
        Button2(uint8_t pin) : ButtonBase(pin) {
            Serial.println("Button2 init");
        }

        virtual ~Button2() {
            detachInterrupt(digitalPinToInterrupt(pin));
            Serial.println("Button2 deinit");
        }

        virtual uint8_t buttonType() const override { return 2; }

        virtual void setup() override {
            pinMode(pin, INPUT_PULLDOWN);
            attachInterrupt(digitalPinToInterrupt(pin), std::bind(&Button2::handleInterrupt, this), RISING);
        }

        virtual void loop() override {
            if (isPressed) {
                unsigned long currentTime = millis();
                if (currentTime - lastTime > debounce) {
                    lastTime = currentTime;
                    isPressed = false;
                    Serial.println("Button pressed");
                }
            }
        }
};
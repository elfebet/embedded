#include "ButtonBase.h"
#include <FunctionalInterrupt.h>

// Завдання 1: Базова реалізація (без debounce)
class Button1 : public ButtonBase {
    private:
        volatile unsigned long pressCount = 0;
        unsigned long lastTime = 0;

        void IRAM_ATTR handleInterrupt() {
            pressCount++;
        }

    public:
        Button1(uint8_t pin) : ButtonBase(pin) {
            Serial.println("Button1 init");
        }

        virtual ~Button1() {
            detachInterrupt(digitalPinToInterrupt(pin));
            Serial.println("Button1 deinit");
        }

        virtual uint8_t buttonType() const override { return 1; }

        virtual void setup() override {
            pinMode(pin, INPUT_PULLDOWN);
            attachInterrupt(digitalPinToInterrupt(pin), std::bind(&Button1::handleInterrupt, this), RISING);
        }

        virtual void loop() override {
            unsigned long currentTime = millis();
            if (currentTime - lastTime >= 1000) {
                lastTime = currentTime;
                Serial.printf("All of interrupts: %lu\n", pressCount);
            }
        }
};
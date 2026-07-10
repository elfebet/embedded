#include "ButtonBase.h"
#include <FunctionalInterrupt.h>

// Завдання 4: Polling + debounce (без interrupts)
class Button4 : public ButtonBase {
    private:
        const unsigned long pollingDelay = 10;
        const unsigned long debounce = 50;
        unsigned long lastDebounceTime = 0;
        unsigned long lastPollTime = 0;
        bool lastButtonState = false;
        bool buttonState = false;

    public:
        Button4(uint8_t pin) : ButtonBase(pin) {
            Serial.println("Button4 init");
        }

        virtual ~Button4() {
            Serial.println("Button4 deinit");
        }

        virtual uint8_t buttonType() const override { return 4; }

        virtual void setup() override {
            pinMode(pin, INPUT_PULLDOWN);
            Serial.println("[Button4] setup");
        }

        virtual void loop() override {
            unsigned long currentTime = millis();
            if (currentTime - lastPollTime >= pollingDelay) {
                lastPollTime = currentTime;

                bool reading = digitalRead(pin);
                if (reading != lastButtonState) {
                    lastDebounceTime = millis();
                }

                if (millis() - lastDebounceTime > debounce && reading != buttonState) {
                    buttonState = reading;
                    if (buttonState == HIGH) {
                        Serial.println("Button pressed");
                    } else {
                        Serial.println("Button released");
                    }
                }

                lastButtonState = reading;
            }   
        }
};
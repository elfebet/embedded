#include <Arduino.h>
#include <FunctionalInterrupt.h>

typedef void (*ButtonCallback)();

class PullDownButton {
private:
    const uint8_t pin;
    
    bool didSetup = false;
    volatile unsigned long lastTime = 0;
    ButtonCallback pressCallback = nullptr;

public:
    PullDownButton(uint8_t pin): pin(pin) {
    }

    ~PullDownButton() {
        if (!didSetup) return;
        detachInterrupt(digitalPinToInterrupt(pin));
    }

    void setup() {
        pinMode(pin, INPUT_PULLDOWN);
        attachInterrupt(digitalPinToInterrupt(pin), [this]() {
            this->handleButtonPress();
        }, RISING);
        didSetup = true;
    }

    void onPress(ButtonCallback callback) {
        pressCallback = callback;
    }

private:
    void IRAM_ATTR handleButtonPress() {
        unsigned long currentTime = millis();
        if (currentTime - lastTime < 200) return;
        
        lastTime = currentTime;
        Serial.println("Натиснули кнопку");
        if (nullptr != pressCallback) {
            pressCallback();
        }
    }
};
#include <Arduino.h>

constexpr uint8_t LED_PIN1 = 4;
constexpr uint8_t LED_PIN2 = 5;
constexpr uint8_t LED_PIN3 = 6;
constexpr uint8_t NUM_LEDS = 3;

struct Led {
  uint8_t pin;
  uint32_t delay;
  unsigned long lastUpdateMs;
  bool isOn;

  Led(uint8_t pin, uint32_t delay, unsigned long lastUpdateMs = 0, bool isOn = false)
      : pin(pin), delay(delay), lastUpdateMs(lastUpdateMs), isOn(isOn) {}
};

Led leds[NUM_LEDS] = {
    Led{LED_PIN1, 200},
    Led{LED_PIN2, 500},
    Led{LED_PIN3, 1000},
};

void setup() {
  for (uint8_t i = 0; i < NUM_LEDS; ++i) {
    pinMode(leds[i].pin, OUTPUT);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  for (uint8_t i = 0; i < NUM_LEDS; ++i) {
    if (currentMillis - leds[i].lastUpdateMs >= leds[i].delay) {
      leds[i].isOn = !leds[i].isOn;
      digitalWrite(leds[i].pin, leds[i].isOn ? HIGH : LOW);
      leds[i].lastUpdateMs = currentMillis;
    }
  }  
}
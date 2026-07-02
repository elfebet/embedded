#include <Arduino.h>
#include "config.h"
#include "Led.h"

unsigned long iterationCount = 0;
unsigned long iterationStartTime = 0;

volatile bool buttonPressed = false;
Led led(LedState::Blinking);

void IRAM_ATTR onButtonPress() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // усуваємо брязкіт між перериваннями
  if (interruptTime - lastInterruptTime > 100) {
    buttonPressed = true;
  }

  lastInterruptTime = interruptTime;
}

void setup() {
  Serial.begin(115200);

  led.setup();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);

  iterationStartTime = millis(); // Початковий час для вимірювання тривалості ітерацій
}

void loop() {
  iterationCount++;

  led.loop();

  if (buttonPressed) {
    buttonPressed = false;

    switch (led.getState()) {
      case LedState::Off:
          led.setState(LedState::On);
          break;
      case LedState::On:
          led.setState(LedState::Blinking);
          break;
      case LedState::Blinking:
          led.setState(LedState::Off);
          break;
    }
  }

  if (iterationCount % 1000000 == 0) {
    float averageTime = (float)(millis() - iterationStartTime) / iterationCount;
    Serial.printf("Середній час 1 ітерації: %.20f, мс.\n", averageTime);
    iterationStartTime = millis();
  }
}
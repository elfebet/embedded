#include <Arduino.h>

const uint8_t BUTTON_PIN = 12;
const uint8_t LED_PIN = 6;

volatile uint32_t interruptCount = 0;
uint32_t lastSentTime= 0;

void setup() {
  Serial.begin(115200);
  delay(3000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), []() {
    digitalWrite(LED_PIN, digitalRead(BUTTON_PIN));
    interruptCount++;
  }, CHANGE);

  Serial.println("Setup complete. Press the button to toggle the LED.");
}

void loop() {
  uint32_t currentTime = millis();
  if (currentTime - lastSentTime >= 500) {
    if (interruptCount > 0) {
      Serial.printf("Count of interrupt events (contact bounce) %lu times.\n", interruptCount);
      interruptCount = 0;
    }
    lastSentTime = currentTime;
  }
}

#include <Arduino.h>

#ifdef ESP32_BOARD
    constexpr uint8_t LED1_PIN = 5;
    constexpr uint8_t LED2_PIN = 6;
    constexpr uint8_t BTN_PIN = 7;
#else // STM32_BOARD
    constexpr uint8_t LED1_PIN = PB0;
    constexpr uint8_t LED2_PIN = PB1;
    constexpr uint8_t BTN_PIN = PA0;
#endif

volatile unsigned long lastPress = 0;
volatile bool newPress = 0;
unsigned long lastBlink = 0;
bool blinkState = false;
bool indicator = false;

void buttonISR() {
  unsigned long now = millis();
  if (now - lastPress > 200) {
    lastPress = now;
    newPress = true;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  pinMode(BTN_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), buttonISR, RISING);

  #ifdef ESP32_BOARD
    Serial.println("Run on esp32 platform");  
  #else // STM32_BOARD
    Serial.println("Run on esp32 platform");
  #endif
}

void loop() {
  uint32_t now = millis();

  if (now - lastBlink > 1000) {
    lastBlink = now;
    blinkState = !blinkState;
    digitalWrite(LED1_PIN, blinkState);
  }

  if (newPress) {
    newPress = false;
    indicator = !indicator;
    digitalWrite(LED2_PIN, indicator);
  }
}
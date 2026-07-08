#include <Arduino.h>

constexpr uint8_t POT_PIN = 4; // potentiometer pin
constexpr uint8_t RELAY_PIN = 10;
constexpr uint8_t RELAY_ON = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

constexpr uint32_t RELAY_DELAY = 1000; // ms
constexpr uint32_t POT_DELAY = 500; // ms

constexpr uint8_t LED_PIN = 7; // LED pin

constexpr int pwmFreq = 5000;    // 5 kHz frequency
constexpr int pwmResolution = 8; // 8-bit resolution (0..255), 12-bit resolution (0..4095)
constexpr int pwmChannel = 0;

bool relayIsOn = false;
unsigned long lastRelayToggleTime = 0;
unsigned long lastPotReadTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  ledcSetup(pwmChannel, pwmFreq, pwmResolution); 
  ledcAttachPin(LED_PIN, pwmChannel); 

  analogReadResolution(pwmResolution); 
  Serial.println("Перемикаємо реле напряму з піна + читаємо потенціометр (аналогове значення) з піна 4");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastRelayToggleTime >= RELAY_DELAY) {
    lastRelayToggleTime = currentTime;
    relayIsOn = !relayIsOn;
    digitalWrite(RELAY_PIN, relayIsOn ? RELAY_ON : RELAY_OFF);
    Serial.printf("Relay: %s\n", relayIsOn ? "ON" : "OFF");
  }

  int val = analogRead(POT_PIN);
  ledcWrite(pwmChannel, val);

  if (currentTime - lastPotReadTime >= POT_DELAY) {
    lastPotReadTime = currentTime;
    Serial.printf("Pot: %d, val: %d\n", val);
  }
}
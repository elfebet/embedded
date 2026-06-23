#include <Arduino.h>
#include "AnalogReader.cpp"

#define LDR_PIN 4
#define LED_PIN 7
#define THRESHOLD 1800

AnalogReader analogReader(LDR_PIN, 3.3);

void setup() {
  Serial.begin(115200);
  analogReader.setup();

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  analogReader.read();

  Serial.printf("Analog: %d, avg analog: %d, milliVolts: %d, voltage: %.3f V, error voltage: %.2f %%\n",
     analogReader.rawValue,
     analogReader.avgRawValue,
     analogReader.milliVoltsValue,
     analogReader.avgVoltage(),
     analogReader.percentageErrorVoltage()
  );

  if (analogReader.avgRawValue < THRESHOLD ) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  delay(100);
}

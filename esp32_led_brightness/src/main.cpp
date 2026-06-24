#include <Arduino.h>
#include "AnalogReader.cpp"

const int photoPin = 4; // Photoresistor pin
const int ledPin = 7;   // LED pin

// PWM properties
const int pwmFreq = 5000;    // 5 kHz frequency
const int pwmResolution = 8; // 8-bit resolution (0 to 255)
const int pwmChannel = 0;

AnalogReader analogReader(photoPin);
int currentBrightness = 0;

void setup() {
  Serial.begin(115200);

  ledcSetup(pwmChannel, pwmFreq, pwmResolution); 
  ledcAttachPin(ledPin, pwmChannel); 
}

void loop() {
  analogReader.read();

  // int brightness = map(analogReader.avgValue, 0, 4095, 255, 0);

  // adjust min/max range (500/3000) based on room lighting
  int brightness = map(analogReader.avgValue, 500, 3000, 255, 0);
  brightness = constrain(brightness, 0, 255);

  // restriction to apply changes
  if (abs(brightness - currentBrightness) <= 2) return;
  currentBrightness = brightness;

  ledcWrite(pwmChannel, currentBrightness);
}

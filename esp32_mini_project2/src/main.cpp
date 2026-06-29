#include <Arduino.h>
#include "PullDownButton.cpp"

const int ldrPin = 4; // фоторезистор
const int headLedPin = 5; // білий світлодіод
const int hazardLedPin1 = 6; // червоний світлодіод
const int hazardLedPin2 = 7; // зелений світлодіод
const int crashButtonPin = 8; // кнопка
const int hazardLedBlinkDelay = 400; // напівтакт аварійки

// PWM properties
const int pwmFreq = 5000;    // 5 kHz frequency
const int pwmResolution = 8; // 8-bit resolution (0 to 255)
const int pwmChannel = 0;

bool isHazard = false;
bool isHazardLedOn = false;
unsigned long lastHazardTime = 0;

PullDownButton button = PullDownButton(crashButtonPin);

void handleButtonPress() {
  isHazard = !isHazard;
}

void setup() {
  Serial.begin(115200);

  // setup ADC for HEAD_PIN
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(headLedPin, pwmChannel);

  pinMode(hazardLedPin1, OUTPUT);
  pinMode(hazardLedPin2, OUTPUT);

  button.setup();
  button.onPress(handleButtonPress);
}

void updateHeadlight() {
  int analogValue = analogRead(ldrPin); // 0..4095

  // adjust min/max range (500/3000) based on room lighting
  int light = map(analogValue, 500, 3000, 255, 0);
  light = constrain(light, 0, 255);

  ledcWrite(pwmChannel, light);
}

void updateHazard() {
  if (isHazard) {
    if (millis() - lastHazardTime >= hazardLedBlinkDelay) {
      lastHazardTime = millis();
      isHazardLedOn = !isHazardLedOn;
      digitalWrite(hazardLedPin1, isHazardLedOn);
      digitalWrite(hazardLedPin2, isHazardLedOn);
    }
  } else {
    digitalWrite(hazardLedPin1, LOW);
    digitalWrite(hazardLedPin2, LOW);
  }
}

void loop() {
  updateHeadlight();
  updateHazard();
}
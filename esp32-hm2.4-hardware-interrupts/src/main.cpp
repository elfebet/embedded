#include <Arduino.h>
#include "Button1.cpp"
#include "Button2.cpp"
#include "Button3.cpp"
#include "Button4.cpp"

#define BOOT_BUTTON_PIN 0
#define BUTTON_PIN 8

ButtonBase *currentButton = nullptr;
volatile bool buttonPressed = 0;

void IRAM_ATTR bootButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), bootButtonPress, FALLING);

  currentButton = new Button1(BUTTON_PIN);
  currentButton->setup();
}

void switchButtonType() {
  uint8_t buttonType = 0;
  if (currentButton) {
    buttonType = currentButton->buttonType();
    delete currentButton;
  }

  switch (buttonType) {
    case 1:
      currentButton = new Button2(BUTTON_PIN);
      break;
    case 2:
      currentButton = new Button3(BUTTON_PIN);
      break;
    case 3:
      currentButton = new Button4(BUTTON_PIN);
      break;
    case 4:
    default:
      currentButton = new Button1(BUTTON_PIN);
      break;
  }
  
  currentButton->setup();
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;
    switchButtonType();
  }

  if (currentButton) {
    currentButton->loop();
  }
}
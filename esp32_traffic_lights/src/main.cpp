#include <Arduino.h>
#include "TrafficLights.hpp"
#include "Button.hpp"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define BUTTON_PIN 0  // BOOT button is on GPIO 0

const uint8_t pins[] = {4, 5, 6, 7};
TrafficLights trafficLights = TrafficLights(pins, ARRAY_SIZE(pins));
Button button = Button(BUTTON_PIN, 50, 1000); // 50ms debounce, 1000ms long press

// Function prototypes for button press handlers
void handleShortPress();
void handleLongPress();

void setup() {
  Serial.begin(115200);
  delay(3000); // Wait for Serial Monitor to open

  trafficLights.setup();
  button.setup();
  button.onShortPress(handleShortPress);
  button.onLongPress(handleLongPress);
}

void loop() {
  button.update();
  trafficLights.update();
}

void handleShortPress() {
  trafficLights.changeSpeed();
}

void handleLongPress() {
  switch (trafficLights.getMode()) {
    case TrafficLights::ALL_OFF:
        trafficLights.setMode(TrafficLights::TRAFFIC_LIGHTS);
        break;
    case TrafficLights::ALL_ON:
        trafficLights.setMode(TrafficLights::ALL_OFF);
        break;
    case TrafficLights::TRAFFIC_LIGHTS:
        trafficLights.setMode(TrafficLights::ALL_ON);
        break;
  }
}
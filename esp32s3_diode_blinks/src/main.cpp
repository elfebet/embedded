#include <Arduino.h>

#define LED_PIN LED_BUILTIN

void setColor(uint8_t pin, uint32_t color);

void setup() {
  pinMode(RGB_BUILTIN, OUTPUT);
}

void loop() {
  setColor(RGB_BUILTIN, 0xFF0000);
  delay(1000);
  setColor(RGB_BUILTIN, 0x0000FF);
  delay(1000);
}

void setColor(uint8_t pin, uint32_t color) {
  uint8_t red = (color >> 16) & 0xFF;
  uint8_t green = (color >> 8) & 0xFF;
  uint8_t blue = color & 0xFF;
  neopixelWrite(pin, red, green, blue);
}
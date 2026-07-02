#pragma once
#include <cstdint>

constexpr uint8_t BUTTON_PIN = 4;
constexpr uint8_t LED_PIN = 6;
constexpr unsigned long LED_BLINKS_MS = 500;
constexpr uint32_t LED_MASK = 1u << LED_PIN;

enum class LedState {
  Off, On, Blinking
};
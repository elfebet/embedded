#pragma once
#include "config.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define BUTTON_DEBOUNCE_MS      25U
#define BUTTON_LONG_PRESS_MS    800U

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    // true: click is reported on release (only if long press did not fire).
    // false: click is reported immediately on press and long press is never reported.
    bool has_long_press;
} button_driver_input_t;

typedef struct {
    button_driver_input_t input;
    uint32_t candidate_since;
    uint32_t pressed_since;
    bool stable_pressed;
    bool candidate_pressed;
    bool long_press_reported;
    bool click_pending;
    bool long_press_pending;
} button_driver_button_t;

typedef struct {
    button_driver_button_t buttons[APP_INPUT_MAX_BUTTONS];
    uint8_t count;
} button_driver_t;

HAL_StatusTypeDef button_driver_init(button_driver_t *buttons, const button_driver_input_t *inputs, uint8_t count);

void button_driver_update(button_driver_t *buttons);
void button_driver_take_events(button_driver_t *buttons, uint32_t *click_events, uint32_t *long_press_events);

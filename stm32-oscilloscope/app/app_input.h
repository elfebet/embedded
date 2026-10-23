#pragma once

#include "input_events.h"
#include "stm32f4xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    input_action_t click;
    input_action_t long_press;
} button_input_data_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    input_value_t value;
} encoder_input_data_t;

HAL_StatusTypeDef app_input_buttons_init(const button_input_data_t *buttons, uint8_t count);
HAL_StatusTypeDef app_input_encoders_init(const encoder_input_data_t *encoders, uint8_t count);

void app_input_poll(void);
void app_input_take_events(input_events_t *events);

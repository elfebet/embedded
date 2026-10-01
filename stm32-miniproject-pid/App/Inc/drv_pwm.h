#pragma once
#include "stm32f4xx_hal.h"

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
} pwm_t;

void pwm_start(const pwm_t *p);  // start from 0%
void pwm_set_duty(const pwm_t *p, float duty); // 0.0..1.0

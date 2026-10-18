#pragma once
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    ADC_HandleTypeDef *adc;
    TIM_HandleTypeDef *tim;
} scope_adc_data_t;

void scope_adc_init(const scope_adc_data_t *scope_adc_data, uint32_t rate_hz);
HAL_StatusTypeDef scope_adc_start(void);
HAL_StatusTypeDef scope_adc_stop(void);

uint32_t scope_adc_sample_rate(void);
HAL_StatusTypeDef scope_adc_set_sample_rate(uint32_t rate_hz);

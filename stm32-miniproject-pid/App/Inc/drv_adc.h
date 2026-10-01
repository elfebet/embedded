#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct {
    ADC_HandleTypeDef *hadc;
    uint8_t oversample; // скільки вимірювань усереднювати (8)
    uint16_t vref_mv;   // reference voltage, mV (3300)
} adc_t;

uint16_t adc_read_mv(const adc_t *a);

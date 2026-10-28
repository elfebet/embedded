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
// Copies the latest samples in chronological order (oldest first).
// total_samples (optional): number of samples captured since the last sample rate change, wraps at 2^32.
HAL_StatusTypeDef scope_adc_copy_latest(uint16_t *destination, uint32_t sample_count, uint32_t *total_samples);

uint32_t scope_adc_sample_rate(void);
HAL_StatusTypeDef scope_adc_set_sample_rate(uint32_t rate_hz);

// Call before scope_adc_start()! measures VDDA from the factory-calibrated Vrefint.
HAL_StatusTypeDef scope_adc_measure_vdda_mv(uint32_t *vdda_mv);

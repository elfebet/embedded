#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>

#define KY040_COUNTS_PER_DETENT 4

typedef struct {
    TIM_HandleTypeDef *timer;
    uint16_t previous_count;
    int32_t partial_counts;
    int64_t pending_detents;
} encoder_driver_t;

HAL_StatusTypeDef encoder_driver_init(encoder_driver_t *encoder, TIM_HandleTypeDef *timer);
void encoder_driver_update(encoder_driver_t *encoder);
void encoder_driver_take_detents(encoder_driver_t *encoder, int32_t *detents);

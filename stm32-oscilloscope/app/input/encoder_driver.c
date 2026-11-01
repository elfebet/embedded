#include "encoder_driver.h"
#include <stddef.h>

HAL_StatusTypeDef encoder_driver_init(encoder_driver_t *encoder, TIM_HandleTypeDef *timer) {
    if (encoder == NULL || timer == NULL) return HAL_ERROR;

    encoder->timer = timer;
    encoder->previous_count = 0;
    encoder->partial_counts = 0;
    encoder->pending_detents = 0;

    const HAL_StatusTypeDef status = HAL_TIM_Encoder_Start(timer, TIM_CHANNEL_ALL);
    if (status != HAL_OK) {
        encoder->timer = NULL;
        return status;
    }

    encoder->previous_count = __HAL_TIM_GET_COUNTER(timer);
    return HAL_OK;
}

void encoder_driver_update(encoder_driver_t *encoder) {
    if (encoder == NULL || encoder->timer == NULL) return;

    const uint16_t current_count = __HAL_TIM_GET_COUNTER(encoder->timer);
    int32_t count_delta = (int32_t)current_count - (int32_t)encoder->previous_count;
    encoder->previous_count = current_count;

    if (count_delta > INT16_MAX) {
        count_delta -= UINT16_MAX + 1;
    } else if (count_delta < INT16_MIN) {
        count_delta += UINT16_MAX + 1;
    }

    encoder->partial_counts += count_delta;
    const int32_t detents = encoder->partial_counts / (int32_t)KY040_COUNTS_PER_DETENT;
    encoder->partial_counts -= detents * (int32_t)KY040_COUNTS_PER_DETENT;
    if (detents > 0 && encoder->pending_detents > INT64_MAX - detents) {
        encoder->pending_detents = INT64_MAX;
    } else if (detents < 0 && encoder->pending_detents < INT64_MIN - detents) {
        encoder->pending_detents = INT64_MIN;
    } else {
        encoder->pending_detents += detents;
    }
}

void encoder_driver_take_detents(encoder_driver_t *encoder, int32_t *detents) {
    if (encoder == NULL || detents == NULL) return;

    if (encoder->pending_detents > INT32_MAX) {
        *detents = INT32_MAX;
    } else if (encoder->pending_detents < INT32_MIN) {
        *detents = INT32_MIN;
    } else {
        *detents = (int32_t)encoder->pending_detents;
    }
    encoder->pending_detents -= *detents;
}

#include "drv_encoder.h"

void encoder_start(const encoder_t *e) {
    HAL_TIM_Encoder_Start(e->htim, TIM_CHANNEL_ALL);
}

int32_t encoder_read_detents(encoder_t *e) {
    uint16_t now = (uint16_t)__HAL_TIM_GET_COUNTER(e->htim); // current cnt
    int16_t diff = (int16_t)(now - e->last_cnt);
    e->last_cnt = now;

    e->remainder += e->invert ? -diff : diff;

    int32_t detents = e->remainder / e->counts_per_detent;
    e->remainder -= detents * e->counts_per_detent;

    return detents;
}

bool encoder_button_is_active(const encoder_button_t *b) {
    bool high = HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_SET;
    return high != true;
}

void encoder_button_init(const encoder_button_t *b) {
    GPIO_InitTypeDef g = {0};
    g.Pin  = b->pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(b->port, &g);
}

bool encoder_button_poll_pressed(encoder_button_t *b) {
    bool raw = encoder_button_is_active(b);

    if (raw == b->stable) {
        b->count = 0;
        return false;
    }
    if (++b->count < b->threshold) {
        return false;
    }
    b->count  = 0;
    b->stable = raw;
    return raw;
}

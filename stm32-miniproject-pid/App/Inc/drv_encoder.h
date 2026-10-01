#pragma once

// Encoder KY-040

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    TIM_HandleTypeDef *htim;         // який таймер (у нас &htim4)
    uint16_t last_cnt;               // значення лічильника при минулому читанні
    int32_t  remainder;              // імпульси, що ще не склали повне клацання
    int32_t  counts_per_detent;      // скільки імпульсів = 1 клацання (4)
    bool     invert;                 // поміняти напрямок
} encoder_t;

// encoder PullUp button
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;

    uint8_t     threshold;   // скільки однакових відліків поспіль потрібно (3)
    uint8_t     count;       // скільки відліків поспіль стан відрізняється від стабільного
    bool        stable;      // підтверджений стан: true = натиснута
} encoder_button_t;

void encoder_start(const encoder_t *e);
/* > 0 — стільки клацань за годинниковою з минулого виклику, < 0 — проти, 0 — не крутили */
int32_t encoder_read_detents(encoder_t *e);

void encoder_button_init(const encoder_button_t *b);
bool encoder_button_poll_pressed(encoder_button_t *b);

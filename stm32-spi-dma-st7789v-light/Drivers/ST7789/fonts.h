#pragma once
#include "stdint.h"

typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *data;
    const uint8_t first_char;
    const uint8_t last_char;
} font_def_t;

extern font_def_t Font_7x10;
extern font_def_t Font_8x12;
extern font_def_t Font_11x18;
extern font_def_t Font_16x26;

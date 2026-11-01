#pragma once
#include "config.h"
#include <stdint.h>

typedef enum {
    GFX_ORIENTATION_PORTRAIT,
    GFX_ORIENTATION_LANDSCAPE,
    GFX_ORIENTATION_PORTRAIT_MIRROR,
    GFX_ORIENTATION_LANDSCAPE_MIRROR
} GFX_Orientation;

void gfx_init(void);

void gfx_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void gfx_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_fill_screen(uint16_t color);

void gfx_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

#if LCD_ENABLE_TESTS
void gfx_tests(void);
#endif

#pragma once
#include <stdint.h>

typedef enum {
    LCD_ORIENTATION_PORTRAIT,
    LCD_ORIENTATION_LANDSCAPE,
    LCD_ORIENTATION_PORTRAIT_MIRROR,
    LCD_ORIENTATION_LANDSCAPE_MIRROR
} LCD_Orientation;

void gfx_init(void);

void gfx_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void gfx_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_fill_screen(uint16_t color);

void gfx_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

void gfx_tests();

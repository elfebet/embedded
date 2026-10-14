#pragma once
#include "fonts.h"
#include <stdint.h>

typedef enum {
	GFX_CHAR_DRAW_MODE_PIXEL = 0,  // very slow, draw by pixel
	GFX_CHAR_DRAW_MODE_LINE = 1,   // fast, draw by line in buffer. if the buffer is small to draw  —> draw in pixel mode
    GFX_CHAR_DRAW_MODE_WORD = 2,   // faster, draw the whole char in buffer. if the buffer is small to draw  —> draw in line mode
} GFX_CharDrawMode;

#define GX_MAX_BUFER_FONT_WIDTH  20
#define GX_MAX_BUFER_FONT_HEIGHT 28
#define PREFERED_CHAR_DRAW_MODE  GFX_CHAR_DRAW_MODE_WORD

void gfx_draw_text(uint16_t x, uint16_t y, const char *str, const font_def_t *font, uint32_t txcolor, uint32_t bgcolor);

void gfx_text_tests(void);

#pragma once
#include "fonts.h"
#include <stdbool.h>
#include <stdint.h>
#include "af_fonts.h"

typedef enum {
	GFX_CHAR_DRAW_MODE_PIXEL = 0,  // very slow, draw by pixel
	GFX_CHAR_DRAW_MODE_LINE = 1,   // fast, draw by line in buffer. if the buffer is small to draw  —> draw in pixel mode
    GFX_CHAR_DRAW_MODE_WORD = 2,   // faster, draw the whole char in buffer. if the buffer is small to draw  —> draw in line mode
} GFX_CharDrawMode;

#define GX_MAX_BUFER_FONT_WIDTH  20
#define GX_MAX_BUFER_FONT_HEIGHT 28
#define PREFERED_CHAR_DRAW_MODE  GFX_CHAR_DRAW_MODE_WORD

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} GFX_TextBounds;

void gfx_draw_text(uint16_t x, uint16_t y, const char *str, const font_def_t *font, uint32_t txcolor, uint32_t bgcolor);

void gfx_adafruit_draw_text(uint16_t x, uint16_t y, const char *str, const GFXfont *font, uint32_t txcolor, uint32_t bgcolor);
bool gfx_adafruit_get_text_bounds(uint16_t x, uint16_t y, const char *str, const GFXfont *font, GFX_TextBounds *bounds);

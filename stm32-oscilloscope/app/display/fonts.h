#pragma once
#include "stdint.h"

typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *data;
    const uint8_t first_char;
    const uint8_t last_char;
} font_def_t;

// AdafruitGFX glyph format
typedef struct {
    uint16_t bitmapOffset; // offset into bitmap[]
    uint8_t width, height; // bitmap size in pixels
    uint8_t xAdvance;      // cursor advance after the glyph
    int8_t xOffset, yOffset; // bitmap UL corner relative to cursor (y = baseline)
} GFXglyph;

// AdafruitGFX font format
typedef struct {
    uint8_t  *bitmap;     // packed 1 bpp glyph bitmaps, MSB first
    GFXglyph *glyph;      // glyphs array
    uint16_t first, last; // first/last ASCII code in the font
    uint8_t yAdvance;     // font row height by default
} GFXfont;


extern font_def_t Font_5x7;
extern font_def_t Font_7x10;
extern font_def_t Font_8x12;
extern font_def_t Font_11x18;
extern font_def_t Font_16x26;

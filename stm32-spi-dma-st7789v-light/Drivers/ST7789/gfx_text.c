#include "gfx_text.h"
#include "config.h"
#include "gfx.h"
#include "st7789_driver.h"
#if LCD_ENABLE_TESTS
#include "af_fonts.h"
#include "colors.h"
#include <stdio.h>
#endif

uint16_t s_font_buffer[GXF_MAX_BUFER_FONT_WIDTH * GXF_MAX_BUFER_FONT_HEIGHT];

static inline GFX_CharDrawMode gfx_char_draw_mode(uint8_t width, uint8_t height) {
    if (PREFERED_CHAR_DRAW_MODE >= GFX_CHAR_DRAW_MODE_WORD && width <= GXF_MAX_BUFER_FONT_WIDTH && height <= GXF_MAX_BUFER_FONT_HEIGHT) {
        return GFX_CHAR_DRAW_MODE_WORD;
    } else if (PREFERED_CHAR_DRAW_MODE >= GFX_CHAR_DRAW_MODE_LINE && width <= GXF_MAX_BUFER_FONT_WIDTH * GXF_MAX_BUFER_FONT_HEIGHT) {
        return GFX_CHAR_DRAW_MODE_LINE;
    } else {
        return GFX_CHAR_DRAW_MODE_PIXEL;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
// ---- UINT16_T FONT ARRAY FORMAT array format ---------

static inline uint64_t fetch_row_bits(const uint16_t **b_ptr, uint8_t words_per_line) {
    const uint16_t *b = *b_ptr;
    uint64_t row_bits = 0;

    switch (words_per_line) {
        case 4: // 64bit
            row_bits = ((uint64_t)b[0] << 48) | ((uint64_t)b[1] << 32) |
                       ((uint64_t)b[2] << 16) | b[3];
            break;
        case 3: // 48bit
            row_bits = ((uint64_t)b[0] << 32) | ((uint64_t)b[1] << 16) | b[2];
            break;
        case 2: // 32bit
            row_bits = ((uint64_t)b[0] << 16) | b[1];
            break;
        case 1:
        default: // 16bit
            row_bits = b[0];
            break;
    }

    // set pointer to the next line
    *b_ptr += words_per_line;
    return row_bits;
}

void gfx_draw_char(uint16_t x, uint16_t y, char ch, const font_def_t *font, uint16_t txcolor, uint16_t bgcolor, GFX_CharDrawMode mode) {
    uint8_t words_per_line = ((font->width - 1) >> 4) + 1; // 1 for <=16, 2 for <=32
    if (words_per_line > 2 || words_per_line == 0) return;

    const uint8_t c = (ch < font->first_char || ch > font->last_char) ? 0 : (ch - font->first_char);
    const uint16_t *b = font->data + (c * words_per_line * font->height);

    // --- WORD MODE: Draw in the buffer ---
    if (mode == GFX_CHAR_DRAW_MODE_WORD) {
        for (uint8_t row = 0; row < font->height; row++) {
            uint64_t row_bits = fetch_row_bits(&b, words_per_line);
            if (font->width <= 16) {
                row_bits = row_bits >> (16 - font->width);
            }
            for (uint8_t col = 0; col < font->width; col++) {
                bool is_on = row_bits & (1UL << (font->width - 1 - col));
                s_font_buffer[row * font->width + col] = is_on ? txcolor : bgcolor;
            }
        }
        st7789_set_window(x, y, x + font->width - 1, y + font->height - 1);
        st7789_write_pixels_blocking(s_font_buffer, font->width * font->height);
        return;
    }
    // --- LINE MODE: Draw by line in the buffer ---
    if (mode == GFX_CHAR_DRAW_MODE_LINE) {
        st7789_set_window(x, y, x + font->width - 1, y + font->height - 1);
        for (uint8_t row = 0; row < font->height; row++) {
            uint64_t row_bits = fetch_row_bits(&b, words_per_line);
            if (font->width <= 16) {
                row_bits = row_bits >> (16 - font->width);
            }
            for (uint8_t col = 0; col < font->width; col++) {
                bool is_on = row_bits & (1UL << (font->width - 1 - col));
                s_font_buffer[col] = is_on ? txcolor : bgcolor;
            }
            st7789_write_pixels_blocking(s_font_buffer, font->width);
        }
        return;
    }
    // --- PIXEL MODE: Draw by pixel ---
    // (mode == GFX_CHAR_DRAW_MODE_PIXEL)
    for (uint8_t row = 0; row < font->height; row++) {
        uint64_t row_bits = fetch_row_bits(&b, words_per_line);
        if (font->width <= 16) {
            row_bits = row_bits >> (16 - font->width);
        }
        for (uint8_t col = 0; col < font->width; col++) {
            bool is_on = row_bits & (1ULL << (font->width - 1 - col));
            st7789_set_window(x + col, y + row, x + col, y + row);
            st7789_draw_pixel(is_on ? txcolor : bgcolor);
        }
    }
}

void gfx_draw_text(uint16_t x, uint16_t y, const char *str, const font_def_t *font, uint32_t txcolor, uint32_t bgcolor) {
    if (!font || !str) return;

    GFX_CharDrawMode mode = gfx_char_draw_mode(font->width, font->height);
    const uint16_t max_w = st7789_width();
    const uint16_t max_h = st7789_height();
    while (*str) {
        if (x + font->width > max_w) {
            x = 0;
            y += font->height;
            if (y + font->height > max_h) {
                break;
            }
        }
        gfx_draw_char(x, y, *str, font, txcolor, bgcolor, mode);
        x += font->width;
        str++;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
// ---- ADAFUIT FONT FORMAT ---------

void gfx_draw_adafruit_char(uint16_t x, uint16_t y, char ch, const GFXfont *font, uint16_t txcolor, uint16_t bgcolor, GFX_CharDrawMode mode) {
    const uint8_t code = (ch < font->first || ch > font->last) ? 0 : (ch - font->first);
    const GFXglyph *g = &((GFXglyph *)font->glyph)[code];
    const uint8_t w = g->width;
    const uint8_t h = g->height;
    const uint16_t x0 = x + g->xOffset;
    const uint16_t y0 = y + g->yOffset;
    const uint8_t *src = font->bitmap + g->bitmapOffset;
    uint8_t bits = 0;
    uint8_t bits_left = 0;

    // --- WORD MODE: Draw in the buffer ---
    if (mode == GFX_CHAR_DRAW_MODE_WORD) {
        const uint32_t glyph_pixels = (uint32_t)w * h;
        for (uint32_t i = 0; i < glyph_pixels; i++) {
            if (bits_left == 0) {
                bits = *src++;
                bits_left = 8;
            }
            s_font_buffer[i] = (bits & 0x80u) ? txcolor : bgcolor;
            bits <<= 1;
            bits_left--;
        }

        st7789_set_window(x0, y0, x0 + w - 1, y0 + h - 1);
        st7789_write_pixels_blocking(s_font_buffer, glyph_pixels);
        return;
    }

    // --- LINE MODE: Draw by line in the buffer ---
    if (mode == GFX_CHAR_DRAW_MODE_LINE) {
        st7789_set_window(x0, y0, x0 + w - 1, y0 + h - 1);
        for (uint8_t row = 0; row < h; row++) {
            for (uint8_t col = 0; col < w; col++) {
                if (bits_left == 0) {
                    bits = *src++;
                    bits_left = 8;
                }
                s_font_buffer[col] = (bits & 0x80) ? txcolor : bgcolor;
                bits <<= 1;
                bits_left--;
            }
            st7789_write_pixels_blocking(s_font_buffer, w);
        }
        return;
    }

    // --- PIXEL MODE: Draw by pixel ---
    // mode == GFX_CHAR_DRAW_MODE_PIXEL
    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            if (bits_left == 0) {
                bits = *src++;
                bits_left = 8;
            }
            st7789_set_window(x0 + col, y0 + row, x0 + col, y0 + row);
            st7789_draw_pixel((bits & 0x80) ? txcolor : bgcolor);
            bits <<= 1;
            bits_left--;
        }
    }
}

void gfx_adafruit_draw_text(uint16_t x, uint16_t y, const char *str, const GFXfont *font, uint32_t txcolor, uint32_t bgcolor) {
    if (!str || !font || !font->glyph) return;

    const int32_t screen_w = st7789_width();
    const int32_t screen_h = st7789_height();
    int16_t cursor_x = x;
    int16_t cursor_y = y;

    while (*str) {
        unsigned char c = *str++;
        if (c == '\n') {
            cursor_x = 0;
            cursor_y += font->yAdvance;
            continue;;
        }

        if (c < font->first || c > font->last) continue;

        GFXglyph *g = &((GFXglyph *)font->glyph)[c - font->first];

        // Враховуємо і правий край гліфа, і крок курсора.
        int32_t glyph_right = cursor_x + g->xOffset + g->width;
        int32_t advance_right = cursor_x + g->xAdvance;
        int32_t right = (glyph_right > advance_right) ? glyph_right : advance_right;
        // Не переносимо повторно перший символ рядка.
        if (cursor_x != 0 && right > screen_w) {
            cursor_x = 0;
            cursor_y += font->yAdvance;
        }

        if (cursor_y >= screen_h) break;

        GFX_CharDrawMode mode = gfx_char_draw_mode(g->width, g->height);
        gfx_draw_adafruit_char(cursor_x, cursor_y, c, font, txcolor, bgcolor, mode);
        cursor_x += g->xAdvance;
    }
}

bool gfx_adafruit_get_text_bounds(uint16_t x, uint16_t y, const char *str, const GFXfont *font, GFX_TextBounds *bounds) {
    if (!str || !font || !bounds) return false;

    int32_t cursor_x = x;
    int32_t cursor_y = y;
    bool found = false;

    while (*str) {
        uint8_t c = (uint8_t)*str++;

        if (c == '\n') {
            cursor_x = x;
            cursor_y += font->yAdvance;
            continue;
        }

        if (c < font->first || c > font->last) continue;

        const GFXglyph *g = &font->glyph[c - font->first];

        if (g->width && g->height) {
            int32_t left   = cursor_x + g->xOffset;
            int32_t top    = cursor_y + g->yOffset;
            int32_t right  = left + g->width  - 1;
            int32_t bottom = top  + g->height - 1;

            if (!found) {
                bounds->x = left;
                bounds->y = top;
                bounds->width = g->width;
                bounds->height = g->height;
                found = true;
            } else {
                int32_t old_right = bounds->x + bounds->width - 1;
                int32_t old_bottom = bounds->y + bounds->height - 1;

                if (left < bounds->x) bounds->x = left;
                if (top < bounds->y) bounds->y = top;
                if (right > old_right) old_right = right;
                if (bottom > old_bottom) old_bottom = bottom;

                bounds->width = old_right - bounds->x + 1;
                bounds->height = old_bottom - bounds->y + 1;
            }
        }

        cursor_x += g->xAdvance;
    }

    return found;
}

//---------------------------------------------------------------------------------------------------------------------------------
// ---- TESTS ---------
#if LCD_ENABLE_TESTS

void gfx_test_font(const font_def_t *font, const char *name) {
    const char* ascii_string = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    uint16_t bgcolor = COLOR_BLACK;
    uint16_t fgcolor = COLOR_WHITE;
    gfx_fill_screen(bgcolor);

    uint32_t start_time = DWT->CYCCNT;
    gfx_draw_text(10, 20, name, font, fgcolor, bgcolor);
    gfx_draw_text(10, 50, "Hello world!", font, fgcolor, bgcolor);
    gfx_draw_text(10, 80, "gymnastic", font, fgcolor, bgcolor);
    gfx_draw_text(10, 110, "V(arg) = 0.25", font, fgcolor, bgcolor);
    gfx_draw_text(10, 140, "V(rest) = 0,33", font, fgcolor, bgcolor);
    gfx_draw_text(10, 180, ascii_string, font, fgcolor, bgcolor);
    uint32_t elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("text1 time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(1000);
}

void gfx_test_fonts() {
    gfx_test_font(&Font_7x10, "7x10");
    gfx_test_font(&Font_8x12, "8x12");
    gfx_test_font(&Font_11x18, "11x18");
    gfx_test_font(&Font_16x26, "16x26");
}

void gfx_test_af_font(const GFXfont *font, const char *name) {
    uint16_t bgcolor = COLOR_BLACK;
    uint16_t fgcolor = COLOR_WHITE;
    gfx_fill_screen(bgcolor);

    uint32_t start_time = DWT->CYCCNT;
    gfx_adafruit_draw_text(10, 40, name, font, fgcolor, bgcolor);
    gfx_adafruit_draw_text(10, 80, "Hello world!", font, fgcolor, bgcolor);
    gfx_adafruit_draw_text(10, 120, "gymnastic", font, fgcolor, bgcolor);
    gfx_adafruit_draw_text(10, 160, "V(arg) = 0.25", font, fgcolor, bgcolor);
    gfx_adafruit_draw_text(10, 200, "V(rest) = 0,33", font, fgcolor, bgcolor);
    uint32_t elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("[%s] time: %lu ms\r\n", name, elapsed_ms);
    HAL_Delay(1000);
}

void gfx_test_af_fonts(void) {
    gfx_test_af_font(&RobotoMonoBold9pt7b, "Bold9pt7b");
    gfx_test_af_font(&RobotoMonoMedium9pt7b, "Medium9pt7b");
    gfx_test_af_font(&RobotoMonoRegular9pt7b, "Regular9pt7b");

    gfx_test_af_font(&RobotoMonoBold10pt7b, "Bold10pt7b");
    gfx_test_af_font(&RobotoMonoMedium10pt7b, "Medium10pt7b");
    gfx_test_af_font(&RobotoMonoRegular10pt7b, "Regular10pt7b");

    gfx_test_af_font(&RobotoMonoBold11pt7b, "Bold11pt7b");
    gfx_test_af_font(&RobotoMonoMedium11pt7b, "Medium11pt7b");
    gfx_test_af_font(&RobotoMonoRegular11pt7b, "Regular11pt7b");

    gfx_test_af_font(&RobotoMonoBold12pt7b, "Bold12pt7b");
    gfx_test_af_font(&RobotoMonoMedium12pt7b, "Medium12pt7b");
    gfx_test_af_font(&RobotoMonoRegular12pt7b, "Regular12pt7b");
}

void gfx_text_tests(void) {
    gfx_test_fonts();
    // gfx_test_af_fonts();
}

#endif

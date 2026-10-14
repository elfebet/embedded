#include "gfx_text.h"
#include "gfx.h"
#include "st7789_driver.h"

uint16_t s_font_buffer[GX_MAX_BUFER_FONT_WIDTH * GX_MAX_BUFER_FONT_HEIGHT];

static inline uint32_t fetch_row_bits_u8(const uint8_t **b_ptr, uint8_t words_per_line) {
    const uint8_t *b = *b_ptr;
    uint32_t row_bits = 0;

    switch (words_per_line) {
        case 4: // 32bit
            row_bits = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                       ((uint32_t)b[2] << 8)  | b[3];
            break;
        case 3: // 24bit
            row_bits = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
            break;
        case 2: // 16bit
            row_bits = ((uint32_t)b[0] << 8) | b[1];
            break;
        case 1:
        default: // 8bit
            row_bits = b[0];
            break;
    }

    // set pointer to the next line
    *b_ptr += words_per_line;
    return row_bits;
}

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


static inline GFX_CharDrawMode gfx_char_draw_mode(uint8_t width, uint8_t height) {
    if (PREFERED_CHAR_DRAW_MODE >= GFX_CHAR_DRAW_MODE_WORD && width <= GX_MAX_BUFER_FONT_WIDTH && height <= GX_MAX_BUFER_FONT_HEIGHT) {
        return GFX_CHAR_DRAW_MODE_WORD;
    } else if (PREFERED_CHAR_DRAW_MODE >= GFX_CHAR_DRAW_MODE_LINE && width <= GX_MAX_BUFER_FONT_WIDTH * GX_MAX_BUFER_FONT_HEIGHT) {
        return GFX_CHAR_DRAW_MODE_LINE;
    } else {
        return GFX_CHAR_DRAW_MODE_PIXEL;
    }
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
            for (uint8_t col = 0; col < font->width; col++) {
                bool is_on = row_bits & (1UL << (font->width - 1 - col));
                s_font_buffer[row * font->width + col] = is_on ? txcolor : bgcolor;
            }
        }
        st7789_set_window(x, y, x + font->width - 1, y + font->height - 1);
        st7789_write_pixels_blocking(s_font_buffer, font->width * font->height);
    }
    // --- LINE MODE: Draw by line in the buffer ---
    else if (mode == GFX_CHAR_DRAW_MODE_LINE) {
        st7789_set_window(x, y, x + font->width - 1, y + font->height - 1);
        for (uint8_t row = 0; row < font->height; row++) {
            uint64_t row_bits = fetch_row_bits(&b, words_per_line);
            for (uint8_t col = 0; col < font->width; col++) {
                bool is_on = row_bits & (1UL << (font->width - 1 - col));
                s_font_buffer[col] = is_on ? txcolor : bgcolor;
            }
            st7789_write_pixels_blocking(s_font_buffer, font->width);
        }
    }
    // --- PIXEL MODE: Draw by pixel ---
    else { // (mode == GFX_CHAR_DRAW_MODE_PIXEL)
        for (uint8_t row = 0; row < font->height; row++) {
            uint64_t row_bits = fetch_row_bits(&b, words_per_line);
            for (uint8_t col = 0; col < font->width; col++) {
                bool is_on = row_bits & (1ULL << (font->width - 1 - col));
                st7789_set_window(x + col, y + row, x + col, y + row);
                st7789_draw_pixel(is_on ? txcolor : bgcolor);
            }
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

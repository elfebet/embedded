#include "gfx.h"
#include "st7789_driver.h"

#define ABS(x) ((x) > 0 ? (x) : -(x))

void gfx_init(void) {
}

void gfx_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= st7789_width() || y >= st7789_height()) return;

    st7789_set_window(x, y, x, y);
    st7789_draw_pixel(color);
}

void gfx_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	uint16_t tmp;
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }

    const uint16_t max_w = st7789_width() - 1;
    const uint16_t max_h = st7789_height() - 1;
	if (x1 > max_w || y1 > max_h) return;
	if (x2 > max_w)  x2 = max_w;
	if (y2 > max_h) y2 = max_h;

    uint32_t pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
    st7789_set_window(x1, y1, x2, y2);

    // ANTON TODO: DEBUG, not resolved yet.
    for (uint32_t i = 0; i < pixels; i++) {
        st7789_draw_pixel(color);
    }
//   st7789_fill_pixels(pixels, color);
}

void gfx_fill_screen(uint16_t color) {
    gfx_fill_rect(0, 0, st7789_width() - 1, st7789_height() - 1, color);
}

void gfx_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	if(x1 == x2 || y1 == y2) {
		gfx_fill_rect(x1, y1, x2, y2, color);
		return;
	}

	int16_t swap;
    uint16_t steep = ABS(y1 - y2) > ABS(x1 - x2);
    if (steep) {
		swap = x1;
		x1 = y1;
		y1 = swap;

		swap = x2;
		x2 = y2;
		y2 = swap;
    }

    if (x1 > x2) {
		swap = x1;
		x1 = x2;
		x2 = swap;

		swap = y1;
		y1 = y2;
		y2 = swap;
    }

    int16_t dx = x2 - x1;
    int16_t dy = ABS(y2 - y1);

    int16_t err = dx / 2;
    int16_t ystep = (y1 < y2) ? 1 : -1;

    for (; x1 <= x2; x1++) {
        if (steep) {
            gfx_draw_pixel(y1, x1, color);
        } else {
            gfx_draw_pixel(x1, y1, color);
        }
        err -= dy;
        if (err < 0) {
            y1 += ystep;
            err += dx;
        }
    }
}

void gfx_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    gfx_draw_line(x1, y1, x2, y1, color);
    gfx_draw_line(x1, y1, x1, y2, color);
    gfx_draw_line(x1, y2, x2, y2, color);
    gfx_draw_line(x2, y1, x2, y2, color);
}

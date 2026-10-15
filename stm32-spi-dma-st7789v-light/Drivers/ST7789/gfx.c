#include "gfx.h"
#include "gfx_text.h"
#include "st7789_driver.h"
#include "colors.h"
#if LCD_ENABLE_TESTS
#include <stdio.h>
#endif

#define ABS(x) ((x) > 0 ? (x) : -(x))

void gfx_init(void) {
      gfx_fill_screen(COLOR_BLACK);
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
    st7789_fill_pixels(color, pixels);
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

void gfx_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data) {
	if (x + w > st7789_width() || y + h > st7789_height()) return;

    st7789_set_window(x, y, x + w - 1, y + h - 1);
    st7789_write_pixels(data, w * h);
}

#if LCD_ENABLE_TESTS

void gfx_tests() {
    uint32_t start_time = DWT->CYCCNT;
    gfx_fill_screen(COLOR_RED);
    uint32_t elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("RED time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(500);

    start_time = DWT->CYCCNT;
    gfx_fill_screen(COLOR_GREEN);
    elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("GREEN time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(500);

    start_time = DWT->CYCCNT;
    gfx_fill_screen(COLOR_BLUE);
    elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("BLUE time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(1000);

    const int screen_w = st7789_width();
    const int screen_h = st7789_height();

    start_time = DWT->CYCCNT;
    for (int i=0; i < screen_w; i += 10) {
        gfx_draw_line(i, 0, i, screen_h, COLOR_WHITE);
    }
    elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("V lines time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(1000);

    start_time = DWT->CYCCNT;
    for (int i=0; i < screen_h; i += 10) {
        gfx_draw_line(0, i, screen_w, i, COLOR_WHITE);
    }
    elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("H lines time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(1000);

    gfx_fill_screen(COLOR_BLACK);

    int w = 100, h = 100;
    int x1 = screen_w/2 - w/2;
    int y1 = screen_h/2 - h/2;
    start_time = DWT->CYCCNT;
    gfx_fill_rect(x1, y1, x1 + w, y1 + h, COLOR_BLUE);
    elapsed_ms = (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
    printf("RECT time: %lu ms\r\n", elapsed_ms);
    HAL_Delay(1000);

    gfx_draw_rect(0, 0, screen_w - 1, screen_h - 1, COLOR_ORANGE);
    HAL_Delay(1000);

    const uint8_t line_width = 3;
    if (line_width > 1) {
        gfx_fill_rect(0, 0, line_width, screen_h-1, COLOR_RED); // left border
        HAL_Delay(500);
        gfx_fill_rect(0, 0, screen_w-1, line_width, COLOR_GREEN); // top border
        HAL_Delay(500);
        gfx_fill_rect(0, screen_h-line_width-1, screen_w-1, screen_h-1, COLOR_BLUE); // bottom border
        HAL_Delay(500);
        gfx_fill_rect(screen_w-line_width-1, 0, screen_w-1, screen_h-1, COLOR_MAGENTA); // right border
        HAL_Delay(500);
    } else {
        gfx_draw_line(0, 0, 0, screen_h-1, COLOR_RED); // left border
        HAL_Delay(500);
        gfx_draw_line(0, 0, screen_w-1, 0, COLOR_GREEN); // top border
        HAL_Delay(500);
        gfx_draw_line(0, screen_h-1, screen_w-1, screen_h-1, COLOR_BLUE); // bottom border
        HAL_Delay(500);
        gfx_draw_line(screen_w-1, 0, screen_w-1, screen_h-1, COLOR_MAGENTA); // right border
        HAL_Delay(500);
    }

    HAL_Delay(3000);
}

#endif

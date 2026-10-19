#include "scope_renderer.h"

#include "colors.h"
#include "gfx.h"
#include "st7789_driver.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SCOPE_GRID_COLOR       0x39E7 // 60, 60, 60
#define SCOPE_GRID_AXIS_COLOR  COLOR_WHITE
#define SCOPE_WAVE_COLOR       COLOR_GREEN
#define SCOPE_BG_COLOR         COLOR_BLACK
#define SCOPE_GROUP_MAX_WIDTH  4U

// For detect the grid position and redraw to right color (wave or grid color)
// This is faster than calculating the grid position mathematically
static bool s_grid_columns[SCOPE_GRID_WIDTH];
static bool s_grid_rows[SCOPE_GRID_HEIGHT];
static uint16_t s_axis_x;
static uint16_t s_axis_y;

static uint16_t s_previous_top[SCOPE_GRID_WIDTH];
static uint16_t s_previous_bottom[SCOPE_GRID_WIDTH];
static uint16_t s_column_pixels[SCOPE_GRID_HEIGHT];
static bool s_wave_has_previous;

static uint16_t s_group_previous_top[SCOPE_GRID_WIDTH];
static uint16_t s_group_previous_bottom[SCOPE_GRID_WIDTH];
static uint16_t s_group_current_top[SCOPE_GRID_WIDTH];
static uint16_t s_group_current_bottom[SCOPE_GRID_WIDTH];
static uint16_t s_group_sample_y[SCOPE_GRID_WIDTH];
static uint16_t s_group_pixels[SCOPE_GROUP_MAX_WIDTH * SCOPE_GRID_HEIGHT];
static bool s_group_has_previous;

void scope_renderer_clear_buffers(void) {
    memset(s_previous_top, 0, sizeof(s_previous_top));
    memset(s_previous_bottom, 0, sizeof(s_previous_bottom));
    memset(s_column_pixels, 0, sizeof(s_column_pixels));
    s_wave_has_previous = false;

    memset(s_group_previous_top, 0, sizeof(s_group_previous_top));
    memset(s_group_previous_bottom, 0, sizeof(s_group_previous_bottom));
    memset(s_group_current_top, 0, sizeof(s_group_current_top));
    memset(s_group_current_bottom, 0, sizeof(s_group_current_bottom));
    memset(s_group_sample_y, 0, sizeof(s_group_sample_y));
    memset(s_group_pixels, 0, sizeof(s_group_pixels));
    s_group_has_previous = false;
}

void scope_renderer_fill_background(void) {
    scope_renderer_clear_buffers();
    gfx_fill_screen(SCOPE_BG_COLOR);
}

void scope_renderer_draw_grid(void) {
    scope_renderer_clear_buffers();

    const uint16_t right = SCOPE_GRID_X + SCOPE_GRID_WIDTH - 1;
    const uint16_t bottom = SCOPE_GRID_Y + SCOPE_GRID_HEIGHT - 1;

    for (uint16_t column = 0; column <= SCOPE_GRID_H_DIVS; ++column) {
        if (column != SCOPE_GRID_H_DIVS / 2) {
            const uint16_t x = SCOPE_GRID_X + column * (SCOPE_GRID_WIDTH - 1) / SCOPE_GRID_H_DIVS;
            gfx_draw_line(x, SCOPE_GRID_Y, x, bottom, SCOPE_GRID_COLOR);
            s_grid_columns[x - SCOPE_GRID_X] = true;
        }
    }
    for (uint16_t row = 0; row <= SCOPE_GRID_V_DIVS; ++row) {
        if (row != SCOPE_GRID_V_DIVS / 2) {
            const uint16_t y = SCOPE_GRID_Y + row * (SCOPE_GRID_HEIGHT - 1) / SCOPE_GRID_V_DIVS;
            gfx_draw_line(SCOPE_GRID_X, y, right, y, SCOPE_GRID_COLOR);
            s_grid_rows[y - SCOPE_GRID_Y] = true;
        }
    }

    s_axis_x = SCOPE_GRID_X + SCOPE_GRID_WIDTH/2 - 1;
    s_axis_y = SCOPE_GRID_Y + SCOPE_GRID_HEIGHT/2 - 1;
    gfx_draw_line(s_axis_x, SCOPE_GRID_Y, s_axis_x, bottom, SCOPE_GRID_AXIS_COLOR);
    gfx_draw_line(SCOPE_GRID_X, s_axis_y, right, s_axis_y, SCOPE_GRID_AXIS_COLOR);
}

static uint16_t grid_background_color(uint16_t screen_x, uint16_t screen_y) {
    if (screen_x == s_axis_x || screen_y == s_axis_y) {
       return SCOPE_GRID_AXIS_COLOR;
    } else if (s_grid_columns[screen_x - SCOPE_GRID_X] || s_grid_rows[screen_y - SCOPE_GRID_Y]) {
        return SCOPE_GRID_COLOR;
    } else {
        return SCOPE_BG_COLOR;
    }
}

void scope_renderer_draw_grid_test(void) {
    uint16_t iter=0;
    for (int i=0; i < SCOPE_GRID_WIDTH; i += 2) {
        gfx_draw_line(SCOPE_GRID_X+i, SCOPE_GRID_Y, SCOPE_GRID_X+i, SCOPE_GRID_Y+SCOPE_GRID_HEIGHT, SCOPE_GRID_COLOR);
        ++iter;
    }
    for (int i=0; i < SCOPE_GRID_HEIGHT; i +=2) {
        gfx_draw_line(SCOPE_GRID_X, SCOPE_GRID_Y+i, SCOPE_GRID_X+SCOPE_GRID_WIDTH, SCOPE_GRID_Y+i, SCOPE_GRID_COLOR);
        ++iter;
    }
    //  55 ітерацій = 118 FPS
    //  74 ітерацій = 89 FPS
    // 110 ітерацій = 60 FPS
    // 150 ітерацій = 42 FPS
    // 180 ітерацій = 38 FPS
    // 220 ітерацій = 30 FPS
}

static uint16_t sample_to_y(uint16_t sample) {
    if (sample > 4095) sample = 4095;
    return SCOPE_GRID_Y + SCOPE_GRID_HEIGHT - 1 - (uint32_t)sample * (SCOPE_GRID_HEIGHT - 1) / 4095;
}

void scope_renderer_draw_wave(const uint16_t *samples, uint32_t sample_count, bool preserve_peaks) {
    if (samples == NULL || sample_count == 0) return;

    const uint16_t graph_width = SCOPE_GRID_WIDTH;
    const uint32_t sample_index_step = sample_count / graph_width;
    const uint32_t sample_index_remainder = sample_count % graph_width;
    uint32_t sample_index = 0;
    uint32_t remainder_accumulator = 0;
    uint16_t previous_y = 0;

    for (uint16_t x = 0; x < graph_width; ++x) {
        uint16_t new_top;
        uint16_t new_bottom;

        if (preserve_peaks && sample_count > graph_width) {
            const uint32_t start = sample_index;
            sample_index += sample_index_step;
            remainder_accumulator += sample_index_remainder;
            if (remainder_accumulator >= graph_width) {
                ++sample_index;
                remainder_accumulator -= graph_width;
            }

            uint16_t sample_min = UINT16_MAX;
            uint16_t sample_max = 0U;
            for (uint32_t i = start; i < sample_index; ++i) {
                if (samples[i] < sample_min) sample_min = samples[i];
                if (samples[i] > sample_max) sample_max = samples[i];
            }

            new_top = sample_to_y(sample_max);
            new_bottom = sample_to_y(sample_min);
        } else {
            const uint16_t sample_y = sample_to_y(samples[sample_index]);
            new_top = x > 0 && previous_y < sample_y ? previous_y : sample_y;
            new_bottom = x > 0 && previous_y > sample_y ? previous_y : sample_y;
            previous_y = sample_y;

            if (x + 1 < graph_width) {
                sample_index += sample_index_step;
                remainder_accumulator += sample_index_remainder;
                if (remainder_accumulator >= graph_width) {
                    ++sample_index;
                    remainder_accumulator -= graph_width;
                }
            }
        }

        if (s_wave_has_previous && new_top == s_previous_top[x] && new_bottom == s_previous_bottom[x]) {
            continue;
        }

        uint16_t draw_top = new_top;
        uint16_t draw_bottom = new_bottom;
        if (s_wave_has_previous) {
            if (s_previous_top[x] < draw_top) draw_top = s_previous_top[x];
            if (s_previous_bottom[x] > draw_bottom) draw_bottom = s_previous_bottom[x];
        }

        const uint16_t screen_x = SCOPE_GRID_X + x;
        for (uint16_t y = draw_top; y <= draw_bottom; ++y) {
            s_column_pixels[y - draw_top] = grid_background_color(screen_x, y);
        }

        for (uint16_t y = new_top; y <= new_bottom; ++y) {
            s_column_pixels[y - draw_top] = SCOPE_WAVE_COLOR;
        }

        st7789_set_window(screen_x, draw_top, screen_x, draw_bottom);
        st7789_write_pixels(s_column_pixels, draw_bottom - draw_top + 1);

        s_previous_top[x] = new_top;
        s_previous_bottom[x] = new_bottom;
    }

    s_wave_has_previous = true;
}

static void prepare_grouped_wave(const uint16_t *samples, uint32_t sample_count) {
    if (sample_count <= SCOPE_GRID_WIDTH) {
        // розподіляємо по колонках і зʼєднуемо сусідні точки
        for (uint16_t x = 0; x < SCOPE_GRID_WIDTH; ++x) {
            const uint32_t sample_index = (uint32_t)x * sample_count / SCOPE_GRID_WIDTH;
            s_group_sample_y[x] = sample_to_y(samples[sample_index]);
            if (x == 0) {
                s_group_current_top[x] = s_group_sample_y[x];
                s_group_current_bottom[x] = s_group_sample_y[x];
            } else {
                const uint16_t previous_y = s_group_sample_y[x - 1];
                const uint16_t current_y = s_group_sample_y[x];
                s_group_current_top[x] = previous_y < current_y ? previous_y : current_y;
                s_group_current_bottom[x] = previous_y > current_y ? previous_y : current_y;
            }
        }
        return;
    }

    // якщо семплів більші ніж колонок
    // шукаємо min/max діапазон щоб не загубити піки для кожної колонки
    for (uint16_t x = 0; x < SCOPE_GRID_WIDTH; ++x) {
        const uint32_t start = (uint32_t)x * sample_count / SCOPE_GRID_WIDTH;
        const uint32_t end = (uint32_t)(x + 1) * sample_count / SCOPE_GRID_WIDTH;
        uint16_t sample_min = UINT16_MAX;
        uint16_t sample_max = 0U;

        for (uint32_t i = start; i < end; ++i) {
            const uint16_t sample = samples[i];
            if (sample < sample_min) sample_min = sample;
            if (sample > sample_max) sample_max = sample;
        }

        s_group_current_top[x] = sample_to_y(sample_max);
        s_group_current_bottom[x] = sample_to_y(sample_min);
    }
}

void scope_renderer_draw_wave_grouped(const uint16_t *samples, uint32_t sample_count) {
    if (samples == NULL || sample_count == 0) return;

    prepare_grouped_wave(samples, sample_count);

    uint16_t x = 0;
    while (x < SCOPE_GRID_WIDTH) {
        // якщо попередній кадр такий самий (не змінився), не перемальовуємо
        if (s_group_has_previous
            && s_group_current_top[x] == s_group_previous_top[x]
            && s_group_current_bottom[x] == s_group_previous_bottom[x]) {
            ++x;
            continue;
        }

        const uint16_t run_start = x;
        uint16_t run_width = 0;
        uint16_t run_top = SCOPE_GRID_Y + SCOPE_GRID_HEIGHT;
        uint16_t run_bottom = SCOPE_GRID_Y - 1;

        // додаємо колонки, доки не дійшли до кінця або макс ширини групи
        while (x < SCOPE_GRID_WIDTH && run_width < SCOPE_GROUP_MAX_WIDTH) {
            const uint16_t current_top = s_group_current_top[x];
            const uint16_t current_bottom = s_group_current_bottom[x];

            // якщо колонка не змінилась, завершуємо групу. незмінні колонки не додаємо до оновлення
            if (s_group_has_previous && current_top == s_group_previous_top[x] && current_bottom == s_group_previous_bottom[x]) {
                break;
            }

            uint16_t draw_top = current_top;
            uint16_t draw_bottom = current_bottom;
            if (s_group_has_previous) {
                if (s_group_previous_top[x] < draw_top) draw_top = s_group_previous_top[x];
                if (s_group_previous_bottom[x] > draw_bottom) draw_bottom = s_group_previous_bottom[x];
            }

            if (draw_top < run_top) run_top = draw_top;
            if (draw_bottom > run_bottom) run_bottom = draw_bottom;

            // зберігаємо нові межі для наступного кадру і переходимо до наступної групи колонок
            s_group_previous_top[x] = current_top;
            s_group_previous_bottom[x] = current_bottom;
            ++x;
            ++run_width;
        }

        // висота прямокутника, який надсилаемо дисплею
        const uint16_t band_height = run_bottom - run_top + 1;
        for (uint16_t row = 0; row < band_height; ++row) {
            const uint16_t screen_y = run_top + row;
            for (uint16_t column = 0U; column < run_width; ++column) {
                const uint16_t sample_x = run_start + column;
                const uint16_t wave_top = s_group_current_top[sample_x];
                const uint16_t wave_bottom = s_group_current_bottom[sample_x];

                const uint16_t screen_x = SCOPE_GRID_X + sample_x;
                const bool wave_pixel = screen_y >= wave_top && screen_y <= wave_bottom;
                s_group_pixels[row * run_width + column] = wave_pixel ? SCOPE_WAVE_COLOR : grid_background_color(screen_x, screen_y);
            }
        }

        st7789_set_window(SCOPE_GRID_X + run_start, run_top, SCOPE_GRID_X + run_start + run_width - 1, run_bottom);
        st7789_write_pixels(s_group_pixels, (uint32_t)run_width * band_height);
    }

    // позначаємо що є попередній кадр
    s_group_has_previous = true;
}

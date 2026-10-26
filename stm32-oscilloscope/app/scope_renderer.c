#include "scope_renderer.h"

#include "app_state.h"
#include "colors.h"
#include "config.h"
#include "fonts.h"
#include "gfx.h"
#include "gfx_text.h"
#include "st7789_driver.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define SCOPE_GRID_COLOR       0x39E7 // 60, 60, 60
#define SCOPE_GRID_AXIS_COLOR  COLOR_WHITE
#define SCOPE_WAVE_COLOR       COLOR_GREEN
#define SCOPE_BG_COLOR         COLOR_BLACK
#define SCOPE_TEXT_COLOR       COLOR_WHITE
#define SCOPE_GROUP_MAX_WIDTH  4U

#define SCOPE_MARKER_COLOR     COLOR_RED
#define SCOPE_MARKER_WIDTH     10U

#define TEXT_COLUMN1_X   0
#define TEXT_COLUMN2_X   160
#define TEXT_LINE1_Y     0
#define TEXT_LINE2_Y     12
#define TEXT_LINE3_Y     24
#define TEXT_LINE4_Y     36
#define TEXT_LINE5_Y     48

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

typedef struct {
    uint32_t sample_rate_hz;
    scope_wave_display_mode_t display_mode;
    vertical_knob_mode_t vertical_knob_mode;
    int32_t offset_y_mv;
    uint8_t volts_div_index;
    uint8_t time_div_index;
    uint8_t fps;
    bool grouped_waves_draw_mode;
    bool running;
} drawn_labels_t;

typedef struct {
    uint32_t vdda_mv;
    uint32_t volts_div_mv;
    int32_t offset_y_mv;
    bool vertical_applied;
} scope_renderer_config_t;

static char s_text_buf[32];
static char s_subtext_buf[16];
static drawn_labels_t s_drawn_labels;
static scope_renderer_config_t s_config;

static inline uint16_t rounded_average(uint32_t sum, uint32_t count) {
    return (uint16_t)((sum + count / 2) / count);
}

void scope_renderer_waves_clear_buffers(void) {
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

void scope_renderer_draw_grid(void) {
    memset(s_grid_columns, 0, sizeof(s_grid_columns));
    memset(s_grid_rows, 0, sizeof(s_grid_rows));
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

static uint16_t sample_to_y(uint16_t sample) {
    if (sample > ADC_MAX_CODE) sample = ADC_MAX_CODE;

    const float sample_mv = (float)sample * (float)s_config.vdda_mv / ADC_MAX_CODE;
    const float vin_mv = (FRONTEND_ZERO_RATIO * (float)s_config.vdda_mv - sample_mv) / FRONTEND_GAIN;
    const float center_y = (float)(SCOPE_GRID_HEIGHT / 2 - 1);
    float y = center_y - (vin_mv + (float)s_config.offset_y_mv) * (SCOPE_GRID_HEIGHT - 1) / ((float)SCOPE_GRID_V_DIVS * (float)s_config.volts_div_mv);

    if (y < 0.0f) y = 0.0f;
    if (y > SCOPE_GRID_HEIGHT - 1) y = SCOPE_GRID_HEIGHT - 1;
    return SCOPE_GRID_Y + (uint16_t)(y + 0.5f);
}

void scope_renderer_reinit(uint32_t vdda_mv) {
    scope_renderer_waves_clear_buffers();
    memset(&s_text_buf, 0, sizeof(s_text_buf));
    memset(&s_subtext_buf, 0, sizeof(s_subtext_buf));
    memset(&s_drawn_labels, 0, sizeof(s_drawn_labels));
    memset(&s_config, 0, sizeof(s_config));

    s_config.vdda_mv = vdda_mv;
    gfx_fill_screen(SCOPE_BG_COLOR);
    scope_renderer_draw_grid();
}

// ----------------- DRAW WAVE -----------------

void scope_renderer_erase_wave(void) {
    if (!s_wave_has_previous) return;

    for (uint16_t x = 0; x < SCOPE_GRID_WIDTH; ++x) {
        const uint16_t top = s_previous_top[x];
        const uint16_t bottom = s_previous_bottom[x];
        const uint16_t screen_x = SCOPE_GRID_X + x;
        for (uint16_t y = top; y <= bottom; ++y) {
            s_column_pixels[y - top] = grid_background_color(screen_x, y);
        }
        st7789_set_window(screen_x, top, screen_x, bottom);
        st7789_write_pixels(s_column_pixels, (uint32_t)bottom - top + 1);
    }
    s_wave_has_previous = false;
}

void scope_renderer_draw_wave(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode) {
    if (samples == NULL || sample_count == 0U) return;

    const uint16_t graph_width = SCOPE_GRID_WIDTH;
    const uint32_t sample_index_step = sample_count / graph_width;
    const uint32_t sample_index_remainder = sample_count % graph_width;
    uint32_t sample_index = 0;
    uint32_t remainder_accumulator = 0;
    uint16_t previous_sample_y = 0;

    for (uint16_t x = 0; x < graph_width; ++x) {
        uint16_t new_top;
        uint16_t new_bottom;

        uint32_t bin_end = sample_index + sample_index_step;
        remainder_accumulator += sample_index_remainder;
        if (remainder_accumulator >= graph_width) {
            ++bin_end;
            remainder_accumulator -= graph_width;
        }

        if (mode == SCOPE_WAVE_DISPLAY_PEAKS && sample_count > graph_width) {
            uint16_t sample_min = UINT16_MAX;
            uint16_t sample_max = 0;
            for (uint32_t i = sample_index; i < bin_end; ++i) {
                if (samples[i] < sample_min) sample_min = samples[i];
                if (samples[i] > sample_max) sample_max = samples[i];
            }
            new_top = sample_to_y(sample_min);
            new_bottom = sample_to_y(sample_max);
        } else {
            uint16_t sample = samples[sample_index];
            if (mode == SCOPE_WAVE_DISPLAY_HI_RES && sample_count > graph_width) {
                uint32_t sum = 0;
                for (uint32_t i = sample_index; i < bin_end; ++i) {
                    sum += samples[i];
                }
                sample = rounded_average(sum, bin_end - sample_index);
            }

            const uint16_t sample_y = sample_to_y(sample);
            new_top = x > 0 && previous_sample_y < sample_y ? previous_sample_y : sample_y;
            new_bottom = x > 0 && previous_sample_y > sample_y ? previous_sample_y : sample_y;
            previous_sample_y = sample_y;
        }

        if (s_wave_has_previous && new_top == s_previous_top[x] && new_bottom == s_previous_bottom[x]) {
            sample_index = bin_end;
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
        sample_index = bin_end;
    }

    s_wave_has_previous = true;
}

// ----------------- DRAW GROUPED WAVE -----------------

void scope_renderer_erase_wave_grouped(void) {
    if (!s_group_has_previous) return;

    for (uint16_t x = 0; x < SCOPE_GRID_WIDTH; ++x) {
        const uint16_t top = s_group_previous_top[x];
        const uint16_t bottom = s_group_previous_bottom[x];
        const uint16_t screen_x = SCOPE_GRID_X + x;
        for (uint16_t y = top; y <= bottom; ++y) {
            s_group_pixels[y - top] = grid_background_color(screen_x, y);
        }
        st7789_set_window(screen_x, top, screen_x, bottom);
        st7789_write_pixels(s_group_pixels, (uint32_t)bottom - top + 1);
    }
    s_group_has_previous = false;
}

static void prepare_grouped_wave(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode) {
    const uint16_t graph_width = SCOPE_GRID_WIDTH;

    if (sample_count <= graph_width || mode == SCOPE_WAVE_DISPLAY_SAMPLE) {
        for (uint16_t x = 0; x < graph_width; ++x) {
            const uint32_t sample_index = (uint32_t)x * sample_count / graph_width;
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

    if (mode == SCOPE_WAVE_DISPLAY_HI_RES) {
        for (uint16_t x = 0; x < graph_width; ++x) {
            const uint32_t start = (uint32_t)x * sample_count / graph_width;
            const uint32_t end = (uint32_t)(x + 1) * sample_count / graph_width;
            uint32_t sum = 0;
            for (uint32_t i = start; i < end; ++i) {
                sum += samples[i];
            }
            const uint16_t avg_sample = rounded_average(sum, end - start);
            s_group_sample_y[x] = sample_to_y(avg_sample);
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

    // SCOPE_WAVE_DISPLAY_PEAKS mode
    // якщо семплів більші ніж колонок
    // шукаємо min/max діапазон щоб не загубити піки для кожної колонки
    for (uint16_t x = 0; x < graph_width; ++x) {
        const uint32_t start = (uint32_t)x * sample_count / graph_width;
        const uint32_t end = (uint32_t)(x + 1) * sample_count / graph_width;
        uint16_t sample_min = UINT16_MAX;
        uint16_t sample_max = 0;
        for (uint32_t i = start; i < end; ++i) {
            if (samples[i] < sample_min) sample_min = samples[i];
            if (samples[i] > sample_max) sample_max = samples[i];
        }
        s_group_current_top[x] = sample_to_y(sample_min);
        s_group_current_bottom[x] = sample_to_y(sample_max);
    }
}

void scope_renderer_draw_wave_grouped(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode) {
    if (samples == NULL || sample_count == 0U) return;

    prepare_grouped_wave(samples, sample_count, mode);

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

// ----------------- DRAW GROUND MARKER -----------------

static void draw_ground_marker(void) {
    const uint16_t x1 = SCOPE_GRID_X - SCOPE_MARKER_WIDTH;
    const uint16_t x2 = SCOPE_GRID_X - 2;
    gfx_fill_rect(x1, SCOPE_GRID_Y - 4, x2, SCOPE_GRID_Y+SCOPE_GRID_HEIGHT-1 + 4, SCOPE_BG_COLOR);

    const float center_y = (float)(SCOPE_GRID_HEIGHT / 2 - 1);
    float y = center_y - (float)s_config.offset_y_mv * (SCOPE_GRID_HEIGHT - 1) / ((float)SCOPE_GRID_V_DIVS * (float)s_config.volts_div_mv);
    if (y < 0.0f) y = 0.0f;
    if (y > SCOPE_GRID_HEIGHT - 1) y = SCOPE_GRID_HEIGHT - 1;
    const uint16_t posY = SCOPE_GRID_Y + (uint16_t)(y + 0.5f);

    for (uint16_t i = 0; i < 4; ++i) {
        gfx_draw_line(x1 + i, posY - (3 - i), x1 + i, posY + (3 - i), SCOPE_MARKER_COLOR);
    }
    gfx_draw_line(x1 + 4, posY, x2, posY, SCOPE_MARKER_COLOR);
}

// ----------------- UPDATE VERTICAL DIV AND OFFSET  -----------------

bool scope_renderer_update_vertical(const app_state_t *state) {
    if (state == NULL) return false;
    const uint32_t volts_div_mv = app_state_volts_div_mv(state);
    if (s_config.vertical_applied && volts_div_mv == s_config.volts_div_mv && state->offset_y_mv == s_config.offset_y_mv) return false;

    s_config.vertical_applied = true;
    s_config.volts_div_mv = volts_div_mv;
    s_config.offset_y_mv = state->offset_y_mv;

    scope_renderer_erase_wave();
    scope_renderer_erase_wave_grouped();
    draw_ground_marker();

    return true;
}

// ----------------- DRAW LABELS -----------------

void scope_renderer_draw_run_stop(const app_state_t *state) {
    if (state == NULL) return;
    if (state->running == s_drawn_labels.running && !state->needs_update) return;
    s_drawn_labels.running = state->running;

    const font_def_t *font = &Font_7x10;
    const uint16_t color = state->running ? COLOR_GREEN : COLOR_RED;
    const uint16_t width = font->width * 5;
    const uint16_t height = 14;
    const uint16_t x = st7789_width() - width;
    const uint16_t ty = (height - font->height) / 2 + 1;

    gfx_fill_rect(x, 0, x + width - 1, height, color);
    if (state->running) {
        const uint16_t tx = x + (width - (font->width * 3)) / 2;
        gfx_draw_text(tx, ty, "RUN", font, COLOR_BLACK, color);
    } else {
        const uint16_t tx = x + (width - (font->width * 4)) / 2;
        gfx_draw_text(tx, ty, "STOP", font, COLOR_WHITE, color);
    }
}

bool scope_renderer_draw_display_mode(const app_state_t *state) {
    if (state == NULL) return false;
    if (state->display_mode == s_drawn_labels.display_mode && !state->needs_update) return false;
    s_drawn_labels.display_mode = state->display_mode;

    const char *mode = app_state_display_mode_label(state);
    snprintf(s_text_buf, sizeof(s_text_buf), "Mode:%-10s", mode);
    gfx_draw_text(TEXT_COLUMN1_X, TEXT_LINE1_Y, s_text_buf, &Font_7x10, SCOPE_TEXT_COLOR, SCOPE_BG_COLOR);
    return true;
}

void scope_renderer_draw_time_div(const app_state_t *state) {
    if (state == NULL) return;
    if (state->time_div_index == s_drawn_labels.time_div_index && !state->needs_update) return;
    s_drawn_labels.time_div_index = state->time_div_index;

    const char *time_div = app_state_time_div_label(state);
    snprintf(s_text_buf, sizeof(s_text_buf), "Time/div:%-10s", time_div);
    gfx_draw_text(TEXT_COLUMN1_X, TEXT_LINE2_Y, s_text_buf, &Font_7x10, SCOPE_TEXT_COLOR, SCOPE_BG_COLOR);
}

void scope_renderer_draw_vertical(const app_state_t *state) {
    if (state == NULL) return;
    if (state->volts_div_index == s_drawn_labels.volts_div_index
        && state->offset_y_mv == s_drawn_labels.offset_y_mv
        && state->vertical_knob_mode == s_drawn_labels.vertical_knob_mode
        && !state->needs_update) return;
    s_drawn_labels.volts_div_index = state->volts_div_index;
    s_drawn_labels.offset_y_mv = state->offset_y_mv;
    s_drawn_labels.vertical_knob_mode = state->vertical_knob_mode;

    // the parameter controlled by the knob is highlighted
    const bool volts_active = state->vertical_knob_mode == VERTICAL_KNOB_VOLTS_DIV;

    snprintf(s_text_buf, sizeof(s_text_buf), "V/div:%-6s", app_state_volts_div_label(state));
    gfx_draw_text(TEXT_COLUMN1_X, TEXT_LINE3_Y, s_text_buf, &Font_7x10, volts_active ? COLOR_YELLOW : SCOPE_TEXT_COLOR, SCOPE_BG_COLOR);

    const int32_t offset = state->offset_y_mv;
    const int32_t offset_abs = offset < 0 ? -offset : offset;
    snprintf(s_text_buf, sizeof(s_text_buf), "Y:%c%ld.%02ldV ", offset < 0 ? '-' : '+', (long)(offset_abs / 1000), (long)((offset_abs % 1000) / 10));
    gfx_draw_text(TEXT_COLUMN1_X, TEXT_LINE4_Y, s_text_buf, &Font_7x10, volts_active ? SCOPE_TEXT_COLOR : COLOR_YELLOW, SCOPE_BG_COLOR);
}

void scope_renderer_draw_sample_rate(const app_state_t *state) {
    if (state == NULL) return;
    if (state->sample_rate_hz == s_drawn_labels.sample_rate_hz && !state->needs_update) return;
    s_drawn_labels.sample_rate_hz = state->sample_rate_hz;

    const uint32_t hz = state->sample_rate_hz;
    if (hz >= 1000000) {
        snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%.1fMHz", (double)hz / 1000000.0);
    } else if (hz >= 1000) {
        if (hz % 1000 == 0) {
            snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lukHz", hz / 1000);
        } else {
            snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%.1fkHz", (double)hz / 1000.0);
        }
    } else {
        snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%luHz", hz);
    }

    snprintf(s_text_buf, sizeof(s_text_buf), "SR:%-12s", s_subtext_buf);
    gfx_draw_text(TEXT_COLUMN2_X, TEXT_LINE1_Y, s_text_buf, &Font_7x10, SCOPE_TEXT_COLOR, SCOPE_BG_COLOR);
}

void scope_renderer_draw_fps(const app_state_t *state) {
    if (state == NULL) return;
    if (state->fps == s_drawn_labels.fps && !state->needs_update) return;
    s_drawn_labels.fps = state->fps;

    snprintf(s_text_buf, sizeof(s_text_buf), "FPS:%-4d", state->fps);
    gfx_draw_text(0, st7789_height()-10, s_text_buf, &Font_7x10, SCOPE_TEXT_COLOR, SCOPE_BG_COLOR);
}

bool scope_renderer_draw_waves_mode(const app_state_t *state) {
    if (state == NULL) return false;
    if (state->grouped_waves_draw_mode == s_drawn_labels.grouped_waves_draw_mode && !state->needs_update) return false;
    s_drawn_labels.grouped_waves_draw_mode = state->grouped_waves_draw_mode;

    const char *mode = state->grouped_waves_draw_mode ? "Grouped" : "Normal";
    snprintf(s_text_buf, sizeof(s_text_buf), "%-8s", mode);
    gfx_draw_text(st7789_width() - 50, st7789_height()-10, s_text_buf, &Font_7x10, SCOPE_TEXT_COLOR, SCOPE_BG_COLOR);
    return true;
}

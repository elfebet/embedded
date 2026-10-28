#include "scope_renderer_labels.h"
#include "scope_renderer.h"

#include "app_state.h"
#include "colors.h"
#include "fonts.h"
#include "gfx.h"
#include "gfx_text.h"
#include "st7789_driver.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define SCOPE_TEXT_COLOR       COLOR_WHITE
#define SCOPE_TEXT_BG_COLOR    COLOR_BLACK
#define SCOPE_TEXT_ACTIVE1     COLOR_YELLOW
#define SCOPE_TEXT_ACTIVE2     COLOR_CYAN

#define TEXT_COLUMN1_X   20
#define TEXT_COLUMN2_X   160
#define TEXT_LINE1_Y     0
#define TEXT_LINE2_Y     12
#define TEXT_LINE3_Y     24
#define TEXT_LINE4_Y     36

#define TEXT_BOTTOM_COLUMN1_X     10
#define TEXT_BOTTOM_COLUMN2_X     100
#define TEXT_BOTTOM_COLUMN3_X     170
#define TEXT_BOTTOM_COLUMN4_X     240
#define TEXT_BOTTOM_LINE1_Y       (SCOPE_GRID_Y + SCOPE_GRID_HEIGHT + 6)
#define TEXT_BOTTOM_LINE2_Y       (TEXT_BOTTOM_LINE1_Y + 12)

typedef struct {
    uint32_t sample_rate_hz;
    scope_wave_display_mode_t display_mode;
    vertical_knob_mode_t vertical_knob_mode;
    horizontal_knob_mode_t horizontal_knob_mode;
    int32_t v_position_cdiv;
    uint8_t volts_div_index;
    uint8_t time_div_index;
    bool time_div_active;

    trigger_mode_t trigger_mode;
    trigger_edge_t trigger_edge;
    trigger_position_t trigger_position;
    int32_t trigger_level_mv;
    bool trigger_level_active;

    int16_t vin_pp;
    int16_t vin_min;
    int16_t vin_max;
    int16_t vin_avg;

    uint32_t freq;
    uint32_t period;
    uint16_t duty;

    uint8_t fps;
    bool grouped_waves_draw_mode;
    bool running;

    bool trigger_status_valid;
    trigger_status_t trigger_status;
} drawn_labels_t;

static char s_text_buf[32];
static char s_subtext_buf[16];
static drawn_labels_t s_drawn_labels;

void scope_renderer_labels_clear_buffer(void) {
    memset(&s_text_buf, 0, sizeof(s_text_buf));
    memset(&s_subtext_buf, 0, sizeof(s_subtext_buf));
    memset(&s_drawn_labels, 0, sizeof(s_drawn_labels));
    // for forcing a redraw of frequency, period, and duty labels
    s_drawn_labels.freq = UINT32_MAX;
    s_drawn_labels.period = UINT32_MAX;
    s_drawn_labels.duty = UINT16_MAX;
}

static inline void draw_label(uint16_t x, uint16_t y, const char *text) {
    gfx_draw_text(x, y, text, &Font_7x10, SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);
}

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
    snprintf(s_text_buf, sizeof(s_text_buf), "DMode:%-10s", mode);
    draw_label(TEXT_COLUMN2_X, TEXT_LINE1_Y, s_text_buf);
    return true;
}

void scope_renderer_draw_vertical(const app_state_t *state) {
    if (state == NULL) return;
    if (state->volts_div_index == s_drawn_labels.volts_div_index
        && state->v_position_cdiv == s_drawn_labels.v_position_cdiv
        && state->vertical_knob_mode == s_drawn_labels.vertical_knob_mode
        && !state->needs_update) return;

    const vertical_knob_mode_t knob_mode = state->vertical_knob_mode;
    s_drawn_labels.volts_div_index = state->volts_div_index;
    s_drawn_labels.v_position_cdiv = state->v_position_cdiv;
    s_drawn_labels.vertical_knob_mode = knob_mode;

    snprintf(s_text_buf, sizeof(s_text_buf), "V/div:%-6s", app_state_volts_div_label(state));
    gfx_draw_text(TEXT_COLUMN2_X, TEXT_LINE2_Y, s_text_buf, &Font_7x10,
         knob_mode == VERTICAL_KNOB_VOLTS_DIV ? SCOPE_TEXT_ACTIVE1 : SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);

    const int32_t pos = state->v_position_cdiv;
    const int32_t pos_abs = pos < 0 ? -pos : pos;
    snprintf(s_text_buf, sizeof(s_text_buf), "VPos:%c%ld.%lddiv ",
            pos < 0 ? '-' : '+',
            pos_abs / V_POSITION_DIV_SCALE,
            (pos_abs % V_POSITION_DIV_SCALE) / 10);
    gfx_draw_text(TEXT_COLUMN2_X, TEXT_LINE3_Y, s_text_buf, &Font_7x10,
         knob_mode == VERTICAL_KNOB_VERTICAL_POSITION ? SCOPE_TEXT_ACTIVE1 : SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);
}

void scope_renderer_draw_time_div(const app_state_t *state) {
    if (state == NULL) return;

    const bool timediv_active = state->horizontal_knob_mode == HORIZONTAL_KNOB_TIME_DIV;
    if (state->time_div_index == s_drawn_labels.time_div_index
        && timediv_active == s_drawn_labels.time_div_active
        && !state->needs_update) return;
    s_drawn_labels.time_div_index = state->time_div_index;
    s_drawn_labels.time_div_active = timediv_active;

    const char *time_div = app_state_time_div_label(state);
    snprintf(s_text_buf, sizeof(s_text_buf), "Time/div:%-7s", time_div);
    gfx_draw_text(TEXT_COLUMN2_X, TEXT_LINE4_Y, s_text_buf, &Font_7x10,
        timediv_active ? SCOPE_TEXT_ACTIVE2 : SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);
}

void scope_renderer_draw_trigger_mode(const app_state_t *state) {
    if (state == NULL) return;
    if (state->trigger_mode == s_drawn_labels.trigger_mode && !state->needs_update) return;
    s_drawn_labels.trigger_mode = state->trigger_mode;

    snprintf(s_text_buf, sizeof(s_text_buf), "TMode:%-6s", app_state_trigger_mode_label(state));
    draw_label(TEXT_COLUMN1_X, TEXT_LINE1_Y, s_text_buf);
}

void scope_renderer_draw_trigger_edge(const app_state_t *state) {
    if (state == NULL) return;
    if (state->trigger_edge == s_drawn_labels.trigger_edge && !state->needs_update) return;
    s_drawn_labels.trigger_edge = state->trigger_edge;

    snprintf(s_text_buf, sizeof(s_text_buf), "TEdge:%-7s", app_state_trigger_edge_label(state));
    draw_label(TEXT_COLUMN1_X, TEXT_LINE2_Y, s_text_buf);
}

void scope_renderer_draw_trigger_position(const app_state_t *state) {
    if (state == NULL) return;
    if (state->trigger_position == s_drawn_labels.trigger_position && !state->needs_update) return;
    s_drawn_labels.trigger_position = state->trigger_position;

    snprintf(s_text_buf, sizeof(s_text_buf), "TPosition:%lu%%", app_state_trigger_position_percent(state));
    draw_label(TEXT_COLUMN1_X, TEXT_LINE3_Y, s_text_buf);
}

void scope_renderer_draw_trigger_level(const app_state_t *state) {
    if (state == NULL) return;

    const bool level_active = state->horizontal_knob_mode == HORIZONTAL_KNOB_TRIGGER_LEVEL;
    if (state->trigger_level_mv == s_drawn_labels.trigger_level_mv
         && level_active == s_drawn_labels.trigger_level_active
         && !state->needs_update) return;
    s_drawn_labels.trigger_level_mv = state->trigger_level_mv;
    s_drawn_labels.trigger_level_active = level_active;

    const int32_t level = state->trigger_level_mv;
    const int32_t level_abs = level < 0 ? -level : level;
    snprintf(s_text_buf, sizeof(s_text_buf), "TLevel:%c%ld.%02ldV ",
        level < 0 ? '-' : '+', level_abs / 1000, (level_abs % 1000) / 10);
    gfx_draw_text(TEXT_COLUMN1_X, TEXT_LINE4_Y, s_text_buf, &Font_7x10,
         level_active ? SCOPE_TEXT_ACTIVE2 : SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);
}

void scope_renderer_draw_trigger_status(const app_state_t *state) {
    if (state == NULL) return;
    if (s_drawn_labels.trigger_status_valid && state->trigger_status == s_drawn_labels.trigger_status && !state->needs_update) return;
    s_drawn_labels.trigger_status_valid = true;
    s_drawn_labels.trigger_status = state->trigger_status;

    const char *text = "ROLL";
    uint16_t color = SCOPE_TEXT_COLOR;
    switch (state->trigger_status) {
    case TRIGGER_STATUS_WAIT:  text = "WAIT"; color = COLOR_YELLOW; break;
    case TRIGGER_STATUS_TRIGD: text = "TRIG"; color = COLOR_GREEN; break;
    case TRIGGER_STATUS_AUTO:  text = "AUTO"; color = COLOR_CYAN; break;
    default: break;
    }
    snprintf(s_text_buf, sizeof(s_text_buf), "%-5s", text);
    gfx_draw_text(st7789_width() - 5 * Font_7x10.width, TEXT_LINE4_Y, s_text_buf, &Font_7x10, color, SCOPE_TEXT_BG_COLOR);
}

void scope_renderer_draw_signal_metrics(const signal_metrics_t *metrics) {
    if (metrics == NULL) return;

    if (metrics->valid) {
        if (s_drawn_labels.duty != metrics->duty_pct) {
            s_drawn_labels.duty = metrics->duty_pct;

            snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%u%%", metrics->duty_pct);
            snprintf(s_text_buf, sizeof(s_text_buf), "DC:%-5s", s_subtext_buf);
            draw_label(TEXT_BOTTOM_COLUMN3_X, TEXT_BOTTOM_LINE2_Y, s_text_buf);
        }

        const uint32_t freq = metrics->freq_hz;
        if (s_drawn_labels.freq != freq) {
            s_drawn_labels.freq = freq;

            if (freq >= 1000000) {
                snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lu.%02luMHz", freq / 1000000, (freq % 1000000) / 10000);
            } else if (freq >= 1000) {
                snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lu.%03lukHz", freq / 1000, freq % 1000);
            } else {
                snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%luHz", freq);
            }
            snprintf(s_text_buf, sizeof(s_text_buf), "F:%-10s", s_subtext_buf);
            draw_label(TEXT_BOTTOM_COLUMN4_X, TEXT_BOTTOM_LINE1_Y, s_text_buf);
        }

        const uint32_t period = metrics->period_us;
        if (s_drawn_labels.period != period) {
            s_drawn_labels.period = period;

            if (period >= 1000000) {
                snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lu.%03lus", period / 1000000, (period % 1000000) / 1000);
            } else if (period >= 1000) {
                snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lu.%03lums", period / 1000, period % 1000);
            } else {
                snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%luus", period);
            }
            snprintf(s_text_buf, sizeof(s_text_buf), "T:%-10s", s_subtext_buf);
            draw_label(TEXT_BOTTOM_COLUMN4_X, TEXT_BOTTOM_LINE2_Y, s_text_buf);
        }
    } else {
        if (s_drawn_labels.freq > 0 || s_drawn_labels.period > 0 || s_drawn_labels.duty > 0) {
            s_drawn_labels.freq = 0;
            s_drawn_labels.period = 0;
            s_drawn_labels.duty = 0;

            snprintf(s_text_buf, sizeof(s_text_buf), "DC:%-5s", "-----");
            draw_label(TEXT_BOTTOM_COLUMN3_X, TEXT_BOTTOM_LINE2_Y, s_text_buf);
            snprintf(s_text_buf, sizeof(s_text_buf), "F:%-10s", "-----");
            draw_label(TEXT_BOTTOM_COLUMN4_X, TEXT_BOTTOM_LINE1_Y, s_text_buf);
            snprintf(s_text_buf, sizeof(s_text_buf), "T:%-10s", "-----");
            draw_label(TEXT_BOTTOM_COLUMN4_X, TEXT_BOTTOM_LINE2_Y, s_text_buf);
        }
   }
}

void scope_renderer_draw_sample_rate(const app_state_t *state) {
    if (state == NULL) return;
    if (state->sample_rate_hz == s_drawn_labels.sample_rate_hz && !state->needs_update) return;
    s_drawn_labels.sample_rate_hz = state->sample_rate_hz;

    const uint32_t hz = state->sample_rate_hz;
    if (hz >= 1000000) {
        snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lu.%luMHz", hz / 1000000, (hz % 1000000) / 100000);
    } else if (hz >= 1000) {
        if (hz % 1000 == 0) snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lukHz", hz / 1000);
        else snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%lu.%lukHz", hz / 1000, (hz % 1000) / 100);
    } else {
        snprintf(s_subtext_buf, sizeof(s_subtext_buf), "%luHz", hz);
    }

    snprintf(s_text_buf, sizeof(s_text_buf), "SR:%-6s", s_subtext_buf);
    draw_label(TEXT_BOTTOM_COLUMN3_X, TEXT_BOTTOM_LINE1_Y, s_text_buf);
}

void scope_renderer_draw_adc_vdda(const adc_metrics_t *metrics) {
    if (metrics == NULL) return;

    const int32_t mvpp = metrics->vin_max - metrics->vin_min;
    if (s_drawn_labels.vin_pp != mvpp) {
        s_drawn_labels.vin_pp = mvpp;

        const int32_t vmpp_abs = mvpp < 0 ? -mvpp : mvpp;
        snprintf(s_text_buf, sizeof(s_text_buf), "Vpp: %c%ld.%03ld",
            mvpp < 0 ? '-' : '+', (long)(vmpp_abs / 1000), (long)(vmpp_abs % 1000));
        draw_label(TEXT_BOTTOM_COLUMN1_X, TEXT_BOTTOM_LINE1_Y, s_text_buf);
    }
    if (s_drawn_labels.vin_avg != metrics->vin_avg) {
        const int32_t vavg = metrics->vin_avg;
        s_drawn_labels.vin_avg = vavg;

        const int32_t vavg_abs = vavg < 0 ? -vavg : vavg;
        snprintf(s_text_buf, sizeof(s_text_buf), "Vavg:%c%ld.%03ld",
            vavg < 0 ? '-' : '+',(long)(vavg_abs / 1000), (long)(vavg_abs % 1000));
        draw_label(TEXT_BOTTOM_COLUMN1_X, TEXT_BOTTOM_LINE2_Y, s_text_buf);
    }

    if (s_drawn_labels.vin_min != metrics->vin_min) {
        const int32_t vmin = metrics->vin_min;
        s_drawn_labels.vin_min = vmin;

        const int32_t vmin_abs = (vmin < 0 ? -vmin : vmin) + 50;
        snprintf(s_text_buf, sizeof(s_text_buf), "Vmin:%c%ld.%ld",
            vmin < 0 ? '-' : '+', (long)(vmin_abs / 1000), (long)((vmin_abs / 100) % 10));
        draw_label(TEXT_BOTTOM_COLUMN2_X, TEXT_BOTTOM_LINE1_Y, s_text_buf);
    }

    if (s_drawn_labels.vin_max != metrics->vin_max) {
        const int32_t vmax = metrics->vin_max;
        s_drawn_labels.vin_max = vmax;

        const int32_t vmax_abs = (vmax < 0 ? -vmax : vmax) + 50;
        snprintf(s_text_buf, sizeof(s_text_buf), "Vmax:%c%ld.%ld",
            vmax < 0 ? '-' : '+', (long)(vmax_abs / 1000), (long)((vmax_abs / 100) % 10));
        draw_label(TEXT_BOTTOM_COLUMN2_X, TEXT_BOTTOM_LINE2_Y, s_text_buf);
    }
}

bool scope_renderer_draw_waves_mode(const app_state_t *state) {
    if (state == NULL) return false;
    if (state->grouped_waves_draw_mode == s_drawn_labels.grouped_waves_draw_mode && !state->needs_update) return false;
    s_drawn_labels.grouped_waves_draw_mode = state->grouped_waves_draw_mode;

    snprintf(s_text_buf, sizeof(s_text_buf), "%-3s", state->grouped_waves_draw_mode ? "GRP" : "NRM");
    gfx_draw_text(st7789_width()-18, st7789_height()-50, s_text_buf, &Font_5x7,
        SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);
    return true;
}

void scope_renderer_draw_fps(uint8_t fps) {
    if (fps == s_drawn_labels.fps) return;
    s_drawn_labels.fps = fps;

    snprintf(s_text_buf, sizeof(s_text_buf), "%-3d", fps);
    gfx_draw_text(st7789_width()-18, st7789_height()-40, s_text_buf, &Font_5x7,
        SCOPE_TEXT_COLOR, SCOPE_TEXT_BG_COLOR);
}

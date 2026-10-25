#pragma once
#include "config.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCOPE_WAVE_DISPLAY_SAMPLE, // Select one sample per horizontal pixel.
    SCOPE_WAVE_DISPLAY_PEAKS,  // Draw each bin's minimum-to-maximum range.
    SCOPE_WAVE_DISPLAY_HI_RES,  // Average samples in each horizontal pixel bin.
    SCOPE_WAVE_DISPLAY_MODE_COUNT
} scope_wave_display_mode_t;

typedef enum {
    VERTICAL_KNOB_VOLTS_DIV,
    VERTICAL_KNOB_OFFSET_Y,
    VERTICAL_KNOB_MODE_COUNT
} vertical_knob_mode_t;

typedef struct {
    // current values
    uint8_t fps;
    uint32_t adc_vdda_mv;
    uint32_t sample_rate_hz;
    scope_wave_display_mode_t display_mode;
    uint8_t time_div_index;

        // todo
        vertical_knob_mode_t vertical_knob_mode;
        uint8_t volts_div_index;
        int16_t offset_y;
    bool running;


    // previous values
    uint32_t last_sample_rate_hz; // for switch in app

        // // todo
        // vertical_knob_mode_t last_vertical_knob_mode;

    // flag indicating that all values need to be updated
    uint8_t needs_update;
} app_state_t;

void app_state_init(app_state_t *state);

const char *app_state_display_mode_label(const app_state_t *state);
const char *app_state_vertical_knob_label(const app_state_t *state);

uint8_t app_state_next_time_div(const app_state_t *state, int8_t delta); // -1 or +1
const char *app_state_time_div_label(const app_state_t *state);

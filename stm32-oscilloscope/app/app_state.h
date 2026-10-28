#pragma once
#include "config.h"
#include <stdbool.h>
#include <stdint.h>

#define V_POSITION_DIV_SCALE  100

typedef enum {
    SCOPE_WAVE_DISPLAY_SAMPLE, // Select one sample per horizontal pixel.
    SCOPE_WAVE_DISPLAY_PEAKS,  // Draw each bin's minimum-to-maximum range.
    SCOPE_WAVE_DISPLAY_HI_RES,  // Average samples in each horizontal pixel bin.
    SCOPE_WAVE_DISPLAY_MODE_COUNT
} scope_wave_display_mode_t;

typedef enum {
    VERTICAL_KNOB_VOLTS_DIV,
    VERTICAL_KNOB_VERTICAL_POSITION,
    VERTICAL_KNOB_MODE_COUNT
} vertical_knob_mode_t;

typedef enum {
    HORIZONTAL_KNOB_TIME_DIV,
    HORIZONTAL_KNOB_TRIGGER_LEVEL,
    HORIZONTAL_KNOB_MODE_COUNT
} horizontal_knob_mode_t;

typedef enum {
    TRIGGER_EDGE_RISING = 0,
    TRIGGER_EDGE_FALLING,
    TRIGGER_EDGE_COUNT
} trigger_edge_t;

typedef enum {
    TRIGGER_POSITION_25 = 0, // trigger point at 25% of the screen from the left
    TRIGGER_POSITION_50,
    TRIGGER_POSITION_75,
    TRIGGER_POSITION_COUNT
} trigger_position_t;

typedef enum {
    TRIGGER_MODE_ROLL = 0, // no trigger, live scrolling graph
    TRIGGER_MODE_AUTO,
    TRIGGER_MODE_NORMAL,
    TRIGGER_MODE_SINGLE,
    TRIGGER_MODE_COUNT
} trigger_mode_t;

typedef enum {
    TRIGGER_STATUS_ROLL = 0,
    TRIGGER_STATUS_WAIT,  // armed, waiting for the trigger event
    TRIGGER_STATUS_TRIGD, // the displayed frame was captured by a trigger event
    TRIGGER_STATUS_AUTO   // auto timeout, free running without sync
} trigger_status_t;

typedef struct {
    uint32_t adc_vdda_mv;
    uint32_t sample_rate_hz;
    scope_wave_display_mode_t display_mode;
    uint8_t time_div_index;

    vertical_knob_mode_t vertical_knob_mode;
    horizontal_knob_mode_t horizontal_knob_mode;
    int32_t v_position_cdiv; // scaled to 1/V_POSITION_DIV_SCALE div
    uint8_t volts_div_index;

    trigger_mode_t trigger_mode;
    trigger_edge_t trigger_edge;
    trigger_position_t trigger_position;
    int32_t trigger_level_mv; // input voltage, -SCOPE_INPUT_MAX_MV..+SCOPE_INPUT_MAX_MV
    trigger_status_t trigger_status;
    bool single_waiting_trigger;

    bool reseting;
    bool running;
    uint8_t needs_update; // flag indicating that all values need to be updated

    bool grouped_waves_draw_mode;
    bool last_grouped_waves_draw_mode;
} app_state_t;

void app_state_init(app_state_t *state);

const char *app_state_display_mode_label(const app_state_t *state);
const char *app_state_trigger_mode_label(const app_state_t *state);
const char *app_state_trigger_edge_label(const app_state_t *state);
uint32_t app_state_trigger_position_percent(const app_state_t *state);

uint8_t app_state_next_time_div(const app_state_t *state, int8_t delta); // -1 or +1
const char *app_state_time_div_label(const app_state_t *state);
uint32_t app_state_time_div_sample_rate_hz(const app_state_t *state);

uint8_t app_state_next_volts_div(const app_state_t *state, int8_t delta); // -1 or +1
const char *app_state_volts_div_label(const app_state_t *state);
uint32_t app_state_volts_div_mv(const app_state_t *state);

int32_t app_state_next_v_position_cdiv(const app_state_t *state, int8_t delta); // -1 or +1
int32_t app_state_next_trigger_level_mv(const app_state_t *state, int32_t delta);

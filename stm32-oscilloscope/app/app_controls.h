#pragma once

#include "input_events.h"
#include "scope_renderer.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HORIZONTAL_KNOB_TIME_DIV,
    // Placeholder: a second horizontal mode (e.g. trigger level) is not defined yet.
    HORIZONTAL_KNOB_MODE_COUNT
} horizontal_knob_mode_t;

typedef enum {
    VERTICAL_KNOB_VOLTS_DIV,
    VERTICAL_KNOB_OFFSET_Y,
    VERTICAL_KNOB_MODE_COUNT
} vertical_knob_mode_t;

typedef struct {
    scope_wave_display_mode_t display_mode;
    horizontal_knob_mode_t horizontal_knob_mode;
    vertical_knob_mode_t vertical_knob_mode;
    uint8_t time_div_index;
    uint8_t volts_div_index;
    int16_t offset_y;
    bool running;
} scope_settings_t;

void app_controls_init(scope_settings_t *settings);
void app_controls_handle(scope_settings_t *settings, const input_events_t *events);
const char *app_controls_vertical_knob_label(const scope_settings_t *settings);

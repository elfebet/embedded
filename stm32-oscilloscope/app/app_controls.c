#include "app_controls.h"
#include "app_state.h"
#include "input_events.h"
#include "logger.h"
#include <stddef.h>

// Placeholder ranges: time/div and V/div are not applied to the scope yet.
#define VOLTS_DIV_STEPS 6
#define DEFAULT_VOLTS_DIV_INDEX 3
#define OFFSET_Y_LIMIT 100

// static int32_t clamp_i32(int32_t value, int32_t min, int32_t max) {
//     return value < min ? min : (value > max ? max : value);
// }

// void app_controls_init(scope_settings_t *settings) {
//     settings->vertical_knob_mode = VERTICAL_KNOB_VOLTS_DIV;
//     settings->volts_div_index = DEFAULT_VOLTS_DIV_INDEX;
//     settings->offset_y = 0;
// }

void app_controls_handle(app_state_t *state, const input_events_t *events) {
    if (state == NULL || events == NULL) return;

    if (events->action_triggered[INPUT_ACTION_RUN_STOP_TOGGLE]) {
        state->running = !state->running;
    }
    if (events->action_triggered[INPUT_ACTION_DISPLAY_MODE_NEXT]) {
        state->display_mode = (state->display_mode + 1) % SCOPE_WAVE_DISPLAY_MODE_COUNT;
    }

    if (events->action_triggered[INPUT_ACTION_BTN_1]) {
        logger("BTN_1 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_3]) {
        logger("BTN_3 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_ENC_BTN_1]) {
        logger("ENC_BTN_1 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_ENC_BTN_1_LONG_PRESS]) {
        logger("ENC_BTN_1 long pressed");
    }

    const int32_t horizontal_delta = events->value_delta[INPUT_VALUE_HORIZONTAL_KNOB];
    if (horizontal_delta != 0) {
        state->time_div_index = app_state_next_time_div(state, horizontal_delta);
        state->sample_rate_hz = app_state_time_div_sample_rate_hz(state);
    }
    const int32_t vertical_delta = events->value_delta[INPUT_VALUE_VERTICAL_KNOB];
    if (vertical_delta != 0) {
        logger("VERTICAL_KNOB moved: %ld", (long)vertical_delta);
    }

/*
    if (events->action_triggered[INPUT_ACTION_VERTICAL_KNOB_CLICK]) {
        state->vertical_knob_mode =
            (vertical_knob_mode_t)((state->vertical_knob_mode + 1) % VERTICAL_KNOB_MODE_COUNT);
    }
    const int32_t vertical_delta = events->value_delta[INPUT_VALUE_VERTICAL_KNOB];
    switch (state->vertical_knob_mode) {
    case VERTICAL_KNOB_VOLTS_DIV:
        state->volts_div_index = (uint8_t)clamp_i32(
            (int32_t)state->volts_div_index + vertical_delta, 0, VOLTS_DIV_STEPS - 1);
        break;
    case VERTICAL_KNOB_OFFSET_Y:
        state->offset_y = (int16_t)clamp_i32(
            (int32_t)state->offset_y + vertical_delta, -OFFSET_Y_LIMIT, OFFSET_Y_LIMIT);
        break;
    default:
        break;
    }
*/
}

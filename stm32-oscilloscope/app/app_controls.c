#include "app_controls.h"
#include "app_state.h"
#include "input_events.h"
#include "logger.h"
#include <stddef.h>

void app_controls_handle(app_state_t *state, const input_events_t *events) {
    if (state == NULL || events == NULL) return;

    if (events->action_triggered[INPUT_ACTION_RESET]) {
        state->reseting = true;
    }
    if (events->action_triggered[INPUT_ACTION_RUN_STOP_TOGGLE]) {
        // run/stop toggle
        state->running = !state->running;
    }
    if (events->action_triggered[INPUT_ACTION_BTN_3]) {
        logger("BTN_3 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_DISPLAY_MODE_NEXT]) {
        // switch display mode
        state->display_mode = (state->display_mode + 1) % SCOPE_WAVE_DISPLAY_MODE_COUNT;
    }
    if (events->action_triggered[INPUT_ACTION_WAVES_DRAW_MODE_NEXT]) {
        state->grouped_waves_draw_mode = !state->grouped_waves_draw_mode;
    }
    if (events->action_triggered[INPUT_ACTION_VERTICAL_KNOB_MODE]) {
        // switch knob mode: V/div <-> Y-pos
        state->vertical_knob_mode = (state->vertical_knob_mode + 1) % VERTICAL_KNOB_MODE_COUNT;
    }
    if (events->action_triggered[INPUT_ACTION_RESET_VERTICAL_POSITION]) {
        // zero the vertical position
        state->v_position_cdiv = 0;
    }

    if (state->running) {
        const int32_t horizontal_delta = events->value_delta[INPUT_VALUE_HORIZONTAL_KNOB];
        if (horizontal_delta != 0) {
            state->time_div_index = app_state_next_time_div(state, horizontal_delta);
            state->sample_rate_hz = app_state_time_div_sample_rate_hz(state);
        }
    }
    const int32_t vertical_delta = events->value_delta[INPUT_VALUE_VERTICAL_KNOB];
    if (vertical_delta != 0) {
        if (state->vertical_knob_mode == VERTICAL_KNOB_VOLTS_DIV) {
            // the 0V marker keeps its screen position, only the scale changes around it
            state->volts_div_index = app_state_next_volts_div(state, vertical_delta);
        } else {
            state->v_position_cdiv = app_state_next_v_position_cdiv(state, vertical_delta);
        }
    }
}

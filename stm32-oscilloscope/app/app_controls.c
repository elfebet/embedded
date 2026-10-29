#include "app_controls.h"
#include "app_state.h"
#include "input_events.h"
#include <stddef.h>

bool app_controls_handle(app_state_t *state, const input_events_t *events) {
    if (state == NULL || events == NULL) return false;

    bool handled = false;

    if (events->action_triggered[INPUT_ACTION_RESET]) {
        state->reseting = true;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_RUN_STOP_TOGGLE]) {
        state->running = !state->running;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_DISPLAY_MODE_NEXT]) {
        state->display_mode = (state->display_mode + 1) % SCOPE_WAVE_DISPLAY_MODE_COUNT;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_WAVES_DRAW_MODE_NEXT]) {
        state->grouped_waves_draw_mode = !state->grouped_waves_draw_mode;
        handled = true;
    }

    if (events->action_triggered[INPUT_ACTION_TRIGGER_MODE_NEXT]) {
        state->trigger_mode = (state->trigger_mode + 1) % TRIGGER_MODE_COUNT;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_TRIGGER_EDGE_NEXT]) {
        state->trigger_edge = state->trigger_edge == TRIGGER_EDGE_RISING ? TRIGGER_EDGE_FALLING : TRIGGER_EDGE_RISING;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_TRIGGER_POSITION_NEXT]) {
        state->trigger_position = (state->trigger_position + 1) % TRIGGER_POSITION_COUNT;
        handled = true;
    }

    if (events->action_triggered[INPUT_ACTION_VERTICAL_KNOB_MODE]) {
        // switch knob mode: V/div -> Y-pos
        state->vertical_knob_mode = (state->vertical_knob_mode + 1) % VERTICAL_KNOB_MODE_COUNT;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_HORIZONTAL_KNOB_MODE]) {
        // switch knob mode: Time/div -> trigger level
        state->horizontal_knob_mode = (state->horizontal_knob_mode + 1) % HORIZONTAL_KNOB_MODE_COUNT;
        handled = true;
    }

    if (events->action_triggered[INPUT_ACTION_RESET_VERTICAL_POSITION]) {
        state->v_position_cdiv = 0;
        handled = true;
    }
    if (events->action_triggered[INPUT_ACTION_RESET_TRIGGER_LEVEL]) {
        state->trigger_level_mv = DEFAULT_TRIGGER_LEVEL_MV;
        handled = true;
    }

    const int32_t horizontal_delta = events->value_delta[INPUT_VALUE_HORIZONTAL_KNOB];
    if (horizontal_delta != 0) {
        if (state->horizontal_knob_mode == HORIZONTAL_KNOB_TIME_DIV) {
            state->time_div_index = app_state_next_time_div(state, horizontal_delta);
            state->sample_rate_hz = app_state_time_div_sample_rate_hz(state);
            handled = true;
        } else if (state->horizontal_knob_mode == HORIZONTAL_KNOB_TRIGGER_LEVEL) {
            state->trigger_level_mv = app_state_next_trigger_level_mv(state, horizontal_delta);
            handled = true;
        }
    }

    const int32_t vertical_delta = events->value_delta[INPUT_VALUE_VERTICAL_KNOB];
    if (vertical_delta != 0) {
        if (state->vertical_knob_mode == VERTICAL_KNOB_VOLTS_DIV) {
            state->volts_div_index = app_state_next_volts_div(state, vertical_delta);
            handled = true;
        } else if (state->vertical_knob_mode == VERTICAL_KNOB_VERTICAL_POSITION) {
            state->v_position_cdiv = app_state_next_v_position_cdiv(state, vertical_delta);
            handled = true;
        }
    }

    return handled;
}

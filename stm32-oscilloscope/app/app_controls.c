#include "app_controls.h"
#include "logger.h"

// Placeholder ranges: time/div and V/div are not applied to the scope yet.
#define TIME_DIV_STEPS 12
#define VOLTS_DIV_STEPS 6
#define DEFAULT_TIME_DIV_INDEX 6
#define DEFAULT_VOLTS_DIV_INDEX 3
#define OFFSET_Y_LIMIT 100

static int32_t clamp_i32(int32_t value, int32_t min, int32_t max) {
    return value < min ? min : (value > max ? max : value);
}

void app_controls_init(scope_settings_t *settings) {
    settings->display_mode = SCOPE_WAVE_DISPLAY_SAMPLE;
    settings->horizontal_knob_mode = HORIZONTAL_KNOB_TIME_DIV;
    settings->vertical_knob_mode = VERTICAL_KNOB_VOLTS_DIV;
    settings->time_div_index = DEFAULT_TIME_DIV_INDEX;
    settings->volts_div_index = DEFAULT_VOLTS_DIV_INDEX;
    settings->offset_y = 0;
    settings->running = true;
}

void app_controls_handle(scope_settings_t *settings, const input_events_t *events) {
    if (events->action_triggered[INPUT_ACTION_BTN_1]) {
        logger("BTN_1 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_2]) {
        logger("BTN_2 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_3]) {
        logger("BTN_3 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_4]) {
        logger("BTN_4 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_ENC_BTN_1]) {
        logger("ENC_BTN_1 pressed");
    }
    if (events->action_triggered[INPUT_ACTION_ENC_BTN_2]) {
        logger("ENC_BTN_2 pressed");
    }

    if (events->action_triggered[INPUT_ACTION_BTN_1_LONG_PRESS]) {
        logger("BTN_1 long pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_2_LONG_PRESS]) {
        logger("BTN_2 long pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_3_LONG_PRESS]) {
        logger("BTN_3 long pressed");
    }
    if (events->action_triggered[INPUT_ACTION_BTN_4_LONG_PRESS]) {
        logger("BTN_4 long pressed");
    }
    if (events->action_triggered[INPUT_ACTION_ENC_BTN_1_LONG_PRESS]) {
        logger("ENC_BTN_1 long pressed");
    }
    if (events->action_triggered[INPUT_ACTION_ENC_BTN_2_LONG_PRESS]) {
        logger("ENC_BTN_2 long pressed");
    }

    const int32_t horizontal_delta = events->value_delta[INPUT_VALUE_HORIZONTAL_KNOB];
    if (horizontal_delta != 0) {
        logger("HORIZONTAL_KNOB moved: %ld", (long)horizontal_delta);
    }
    const int32_t vertical_delta = events->value_delta[INPUT_VALUE_VERTICAL_KNOB];
    if (vertical_delta != 0) {
        logger("VERTICAL_KNOB moved: %ld", (long)vertical_delta);
    }

/*
    if (events->action_triggered[INPUT_ACTION_RUN_STOP_TOGGLE]) {
        settings->running = !settings->running;
    }
    if (events->action_triggered[INPUT_ACTION_DISPLAY_MODE_NEXT]) {
        settings->display_mode = settings->display_mode == SCOPE_WAVE_DISPLAY_HI_RES
                                     ? SCOPE_WAVE_DISPLAY_SAMPLE
                                     : (scope_wave_display_mode_t)(settings->display_mode + 1);
    }
    if (events->action_triggered[INPUT_ACTION_HORIZONTAL_KNOB_CLICK]) {
        settings->horizontal_knob_mode =
            (horizontal_knob_mode_t)((settings->horizontal_knob_mode + 1) % HORIZONTAL_KNOB_MODE_COUNT);
    }
    if (events->action_triggered[INPUT_ACTION_VERTICAL_KNOB_CLICK]) {
        settings->vertical_knob_mode =
            (vertical_knob_mode_t)((settings->vertical_knob_mode + 1) % VERTICAL_KNOB_MODE_COUNT);
    }

    const int32_t horizontal_delta = events->value_delta[INPUT_VALUE_HORIZONTAL_KNOB];
    switch (settings->horizontal_knob_mode) {
    case HORIZONTAL_KNOB_TIME_DIV:
        settings->time_div_index = (uint8_t)clamp_i32(
            (int32_t)settings->time_div_index + horizontal_delta, 0, TIME_DIV_STEPS - 1);
        break;
    default:
        break;
    }

    const int32_t vertical_delta = events->value_delta[INPUT_VALUE_VERTICAL_KNOB];
    switch (settings->vertical_knob_mode) {
    case VERTICAL_KNOB_VOLTS_DIV:
        settings->volts_div_index = (uint8_t)clamp_i32(
            (int32_t)settings->volts_div_index + vertical_delta, 0, VOLTS_DIV_STEPS - 1);
        break;
    case VERTICAL_KNOB_OFFSET_Y:
        settings->offset_y = (int16_t)clamp_i32(
            (int32_t)settings->offset_y + vertical_delta, -OFFSET_Y_LIMIT, OFFSET_Y_LIMIT);
        break;
    default:
        break;
    }
*/
}

const char *app_controls_vertical_knob_label(const scope_settings_t *settings) {
    return settings->vertical_knob_mode == VERTICAL_KNOB_OFFSET_Y ? "Y-pos" : "V/div";
}

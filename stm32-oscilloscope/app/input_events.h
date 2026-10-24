#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INPUT_ACTION_NONE,
    INPUT_ACTION_RUN_STOP_TOGGLE,
    INPUT_ACTION_DISPLAY_MODE_NEXT,

    INPUT_ACTION_BTN_1,
    INPUT_ACTION_BTN_3,

    INPUT_ACTION_ENC_BTN_1,
    INPUT_ACTION_ENC_BTN_2,
    INPUT_ACTION_ENC_BTN_1_LONG_PRESS,
    INPUT_ACTION_ENC_BTN_2_LONG_PRESS,

    // INPUT_ACTION_HORIZONTAL_KNOB_CLICK,
    // INPUT_ACTION_VERTICAL_KNOB_CLICK,
    // INPUT_ACTION_VERTICAL_KNOB_LONG_PRESS,
    INPUT_ACTION_COUNT
} input_action_t;

typedef enum {
    INPUT_VALUE_HORIZONTAL_KNOB,
    INPUT_VALUE_VERTICAL_KNOB,
    INPUT_VALUE_COUNT
} input_value_t;

// Everything that happened between two input polls.
typedef struct {
    bool action_triggered[INPUT_ACTION_COUNT];
    int32_t value_delta[INPUT_VALUE_COUNT];
} input_events_t;

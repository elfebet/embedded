#include "app_input.h"
#include "button_driver.h"
#include "encoder_driver.h"
#include <string.h>

static button_driver_t s_driver_buttons;
static button_input_data_t s_buttons[APP_INPUT_MAX_BUTTONS];
static uint8_t s_buttons_count;

static encoder_driver_t s_driver_encoders[APP_INPUT_MAX_ENCODERS];
static encoder_input_data_t s_encoders[APP_INPUT_MAX_ENCODERS];
static uint8_t s_encoders_count;

static input_events_t pending_events;

HAL_StatusTypeDef app_input_buttons_init(const button_input_data_t *buttons, uint8_t count) {
    if (buttons == NULL || count == 0 || count > APP_INPUT_MAX_BUTTONS) return HAL_ERROR;
    // memset(&pending_events, 0, sizeof(pending_events));

    button_driver_input_t inputs[count];
    for (uint8_t i = 0; i < count; ++i) {
        inputs[i].port = buttons[i].port;
        inputs[i].pin = buttons[i].pin;
        inputs[i].has_long_press = buttons[i].long_press != 0;
    }
    if (button_driver_init(&s_driver_buttons, inputs, count) != HAL_OK) {
        return HAL_ERROR;
    }

    memcpy(s_buttons, buttons, count * sizeof(button_input_data_t));
    s_buttons_count = count;
    return HAL_OK;
}

HAL_StatusTypeDef app_input_encoders_init(const encoder_input_data_t *encoders, uint8_t count) {
    if (encoders == NULL || count == 0 || count > APP_INPUT_MAX_ENCODERS) return HAL_ERROR;
    // memset(&pending_events, 0, sizeof(pending_events));

    encoder_driver_t encoder_driver[count];
    for (uint8_t i = 0; i < count; ++i) {
        const HAL_StatusTypeDef status = encoder_driver_init(&encoder_driver[i], encoders[i].htim);
        if (status != HAL_OK) return status;
    }

    memcpy(s_encoders, encoders, count * sizeof(encoder_input_data_t));
    memcpy(s_driver_encoders, encoder_driver, count * sizeof(encoder_driver_t));
    s_encoders_count = count;
    return HAL_OK;
}

void app_input_poll(void) {
    for (uint8_t i = 0; i < s_encoders_count; ++i) {
        encoder_driver_update(&s_driver_encoders[i]);
        int32_t detents;
        encoder_driver_take_detents(&s_driver_encoders[i], &detents);
        if (detents != 0) {
            pending_events.value_delta[s_encoders[i].value] += detents;
        }
    }

    uint32_t clicks = 0;
    uint32_t long_presses = 0;
    button_driver_update(&s_driver_buttons);
    button_driver_take_events(&s_driver_buttons, &clicks, &long_presses);
    if (clicks == 0 && long_presses == 0) return;

    for (uint8_t i = 0; i < s_buttons_count; ++i) {
        const uint32_t mask = 1UL << i;
        if ((clicks & mask) != 0 && s_buttons[i].click != INPUT_ACTION_NONE) {
            pending_events.action_triggered[s_buttons[i].click] = true;
        }
        if ((long_presses & mask) != 0 && s_buttons[i].long_press != INPUT_ACTION_NONE) {
            pending_events.action_triggered[s_buttons[i].long_press] = true;
        }
    }
}

void app_input_take_events(input_events_t *events) {
    if (events == NULL) return;
    *events = pending_events;
    memset(&pending_events, 0, sizeof(pending_events));
}

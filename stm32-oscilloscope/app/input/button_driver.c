#include "button_driver.h"
#include <string.h>

static bool is_button_pressed(const button_driver_input_t *input) {
    return HAL_GPIO_ReadPin(input->port, input->pin) == GPIO_PIN_RESET;
}

HAL_StatusTypeDef button_driver_init(button_driver_t *buttons, const button_driver_input_t *inputs, uint8_t count) {
    if (buttons == NULL || inputs == NULL || count == 0 || count > APP_INPUT_MAX_BUTTONS)
        return HAL_ERROR;

    for (uint8_t i = 0; i < count; ++i) {
        const uint16_t pin = inputs[i].pin;
        if (inputs[i].port == NULL || pin == 0 || (pin & (pin - 1)) != 0)
            return HAL_ERROR;
    }

    memset(buttons, 0, sizeof(*buttons));
    buttons->count = count;
    for (uint8_t i = 0; i < count; ++i) {
        button_driver_button_t *button = &buttons->buttons[i];
        button->input = inputs[i];
        const bool pressed = is_button_pressed(&inputs[i]);
        button->stable_pressed = pressed;
        button->candidate_pressed = pressed;
    }

    return HAL_OK;
}

void button_driver_update(button_driver_t *buttons) {
    if (buttons == NULL || buttons->count == 0) return;

    const uint32_t now = HAL_GetTick();
    for (uint8_t i = 0; i < buttons->count; ++i) {
        button_driver_button_t *button = &buttons->buttons[i];
        const bool has_long_press = button->input.has_long_press;
        const bool pressed = is_button_pressed(&button->input);

        if (pressed != button->candidate_pressed) {
            button->candidate_pressed = pressed;
            button->candidate_since = now;
        } else if (pressed != button->stable_pressed && (now - button->candidate_since) >= BUTTON_DEBOUNCE_MS) {
            button->stable_pressed = pressed;
            if (pressed) {
                button->pressed_since = now;
                button->long_press_reported = false;
                if (!has_long_press) {
                    button->click_pending = true;
                }
            } else if (has_long_press && !button->long_press_reported) {
                button->click_pending = true;
            }
        }

        if (has_long_press && button->stable_pressed && !button->long_press_reported && (now - button->pressed_since) >= BUTTON_LONG_PRESS_MS) {
            button->long_press_reported = true;
            button->long_press_pending = true;
        }
    }
}

void button_driver_take_events(button_driver_t *buttons, uint32_t *click_events, uint32_t *long_press_events) {
    if (buttons == NULL || click_events == NULL || long_press_events == NULL) return;

    *click_events = 0;
    *long_press_events = 0;
    for (uint8_t i = 0; i < buttons->count; ++i) {
        button_driver_button_t *button = &buttons->buttons[i];
        if (button->click_pending) {
            *click_events |= (uint32_t)1 << i;
            button->click_pending = false;
        }
        if (button->long_press_pending) {
            *long_press_events |= (uint32_t)1 << i;
            button->long_press_pending = false;
        }
    }
}

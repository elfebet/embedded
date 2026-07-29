#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "soc/gpio_num.h"

#define PANIC_BTN_PIN GPIO_NUM_7

typedef enum {
    BTN_STATE_IDLE = 0,
    BTN_STATE_PRESSED,
    BTN_STATE_HELD,
    BTN_STATE_RELEASE,
} button_state_t;

// EVENT FSM TO GIVE EQUAL ONE TIME FOR CLICKING
typedef enum {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_SHORT_PRESS,
    BTN_EVENT_LONG_PRESS,
} button_event_t;

typedef enum {
    BTN_PULLUP = 0,
    BTN_PULLDOWN,
} button_type_t;

// POD (Plain Old Data)
typedef struct {
    gpio_num_t pin;
    button_type_t type;
    button_state_t state;
    uint32_t state_enter_ms; // label of time entry to current state
    bool long_fired;         // flag "LONG_PRESS_MS"
} button_fsm_t;

void panic_sensor_init(button_type_t type, void (*on_panic_deferred)(void));
void button_fsm_init(button_fsm_t *btn, gpio_num_t pin, button_type_t type);
button_event_t button_fsm_tick(button_fsm_t *btn, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
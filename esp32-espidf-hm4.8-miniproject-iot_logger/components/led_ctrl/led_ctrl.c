#include "led_ctrl.h"
#include "driver/gpio.h"

static uint32_t s_led_pin = 0;

void led_ctrl_init(uint32_t led_pin) {
    s_led_pin = led_pin;
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << led_pin),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(led_pin, 0);
}

void led_ctrl_set(bool on) {
    gpio_set_level(s_led_pin, on ? 1 : 0);
}
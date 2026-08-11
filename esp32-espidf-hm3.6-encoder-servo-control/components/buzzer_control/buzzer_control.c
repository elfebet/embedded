#include "include/buzzer_control.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static esp_timer_handle_t buzzer_timer = NULL;

static void buzzer_off_callback(void* arg) {
    gpio_set_level(BUZZER_GPIO, 0);
}

void buzzer_setup(void) {
    const gpio_config_t conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);
    gpio_set_level(BUZZER_GPIO, 0);

    // create disposable timer
    const esp_timer_create_args_t buzzer_timer_args = {
        .callback = &buzzer_off_callback,
        .name = "buzzer_timer"
    };
    esp_timer_create(&buzzer_timer_args, &buzzer_timer);
}

void buzzer_play(uint32_t duration_ms) {
    gpio_set_level(BUZZER_GPIO, 1); // turn ON sound
    esp_timer_stop(buzzer_timer);
    esp_timer_start_once(buzzer_timer, duration_ms * 1000); 
}
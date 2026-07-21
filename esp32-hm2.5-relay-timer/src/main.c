#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// configure pins
#define FAN_PIN GPIO_NUM_4

// set time in microseconds (1 second = 1 000 000 microseconds)
#define FAN_RUN_TIME_US     (5 * 1000 * 1000) // 5 seconds
#define FAN_OFF_TIME_US     (10 * 1000 * 1000) // 10 seconds
// #define FAN_RUN_TIME_US     (15ULL * 60 * 1000 * 1000) // 15 minutes
// #define FAN_OFF_TIME_US     (45ULL * 60 * 1000 * 1000) // 45 minutes (60 min - 15 min)

static esp_timer_handle_t fan_timer;

static void fan_timer_callback(void* arg) {
    static bool is_running = false;

    if (!is_running) {
        // turn ON
        gpio_set_level(FAN_PIN, 1);
        is_running = true;
        // start one-time timer when fun is ON
        esp_timer_start_once(fan_timer, FAN_RUN_TIME_US);
    } else {
        // turn OFF
        gpio_set_level(FAN_PIN, 0);
        is_running = false;
        // start one-time timer when fun is OFF
        esp_timer_start_once(fan_timer, FAN_OFF_TIME_US);
    }
}

void app_main() {
    // configure GPIO
    gpio_reset_pin(FAN_PIN);
    gpio_set_direction(FAN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(FAN_PIN, 0);

    // configure and create timer
    const esp_timer_create_args_t timer_args = {
        .callback = &fan_timer_callback,
        .name = "fan_timer"
    };
    esp_timer_create(&timer_args, &fan_timer);

    // start logic of timers
    fan_timer_callback(NULL);
}
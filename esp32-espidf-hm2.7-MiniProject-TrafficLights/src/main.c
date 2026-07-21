#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_RED    GPIO_NUM_4
#define LED_YELLOW GPIO_NUM_5
#define LED_GREEN  GPIO_NUM_6

#define RED_DURATION_MS         15000
#define RED_YELLOW_DURATION_MS  1000
#define GREEN_DURATION_MS       15000
#define GREEN_BLINK_DURATION_MS 3000
#define YELLOW_DURATION_MS      2000

#define BLINK_DELAY_SEC 500
#define TOTAL_GREEN_BLINKS (GREEN_BLINK_DURATION_MS / BLINK_DELAY_SEC)

typedef enum {
    STATE_RED,
    STATE_RED_YELLOW,
    STATE_GREEN,
    STATE_GREEN_BLINK,
    STATE_YELLOW
} traffic_state_t;

static const char *TAG = "Traffic_Light";
static traffic_state_t traffic_state = STATE_RED;
static int blink_counter = 0;
static TimerHandle_t traffic_timer = NULL;

static void set_traffic_lights(uint32_t red, uint32_t yellow, uint32_t green) {
    gpio_set_level(LED_RED, red);
    gpio_set_level(LED_YELLOW, yellow);
    gpio_set_level(LED_GREEN, green);
}

static void traffic_light_timer_cb(TimerHandle_t xTimer) {
    uint32_t next_interval_ms = 1000;

    switch (traffic_state) {
        case STATE_RED:
            // switch to red + yellow
            set_traffic_lights(1, 1, 0);
            traffic_state = STATE_RED_YELLOW;
            next_interval_ms = RED_YELLOW_DURATION_MS;
            break;

        case STATE_RED_YELLOW:
            // switch to green
            set_traffic_lights(0, 0, 1);
            traffic_state = STATE_GREEN;
            next_interval_ms = GREEN_DURATION_MS;
            break;

        case STATE_GREEN:
            // switch to green blink
            blink_counter = 0;
            set_traffic_lights(0, 0, 0);
            traffic_state = STATE_GREEN_BLINK;
            next_interval_ms = BLINK_DELAY_SEC;
            break;

        case STATE_GREEN_BLINK:
            blink_counter++;
            if (blink_counter < TOTAL_GREEN_BLINKS) { 
                // blinks green
                uint32_t green_on = (blink_counter % 2 != 0) ? 1 : 0;
                set_traffic_lights(0, 0, green_on);
                next_interval_ms = BLINK_DELAY_SEC;
            } else {
                // switch to yellow
                set_traffic_lights(0, 1, 0);
                traffic_state = STATE_YELLOW;
                next_interval_ms = YELLOW_DURATION_MS;
            }
            break;

        case STATE_YELLOW:
            // switch to red
            set_traffic_lights(1, 0, 0);
            traffic_state = STATE_RED;
            next_interval_ms = RED_DURATION_MS;
            break;
    }

    // restart timer with new interval for next state
    xTimerChangePeriod(traffic_timer, pdMS_TO_TICKS(next_interval_ms), 0);
}

void app_main(void) {
    // configure gpio
    gpio_config_t io_conf = {
        .pin_bit_mask = ((1ULL << LED_RED) | (1ULL << LED_YELLOW) | (1ULL << LED_GREEN)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // setup default pin values
    set_traffic_lights(1, 0, 0);

    //  create one-time timer
    traffic_timer = xTimerCreate(
        "TrafficTimer",
        pdMS_TO_TICKS(RED_DURATION_MS), // start from red state
        pdFALSE,                        // auto-reload is disabled, we restart timer on state change
        (void *)0,
        traffic_light_timer_cb
    );

    // start timer
    if (traffic_timer != NULL) {
        xTimerStart(traffic_timer, 0);
        ESP_LOGI(TAG, "Start traffic light");
    } else {
        ESP_LOGE(TAG, "Failed to start timer!");
    }
}
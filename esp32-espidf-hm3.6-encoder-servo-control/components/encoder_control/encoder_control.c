#include <stdint.h>
#include "include/encoder_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "hal/pcnt_types.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"

#define BUTTON_DEBOUNCE   50000 // 50 ms
#define LONG_PRESS_DELAY  500000// 500 ms

static const char *TAG = "ENCODER_CONTROL";
static encoder_rotation_callback_t s_rotation_callback = NULL;
static encoder_button_callback_t s_short_press_callback = NULL;
static encoder_button_callback_t s_long_press_callback = NULL;

pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_channel = NULL;
QueueHandle_t encoder_queue = NULL;
TaskHandle_t encoder_task = NULL;

typedef enum {
    ENCODER_ROTATION,
    ENCODER_BUTTON_PRESSED,
    ENCODER_BUTTON_RELEASED
} encoder_event_type_t;

typedef struct {
    encoder_event_type_t type;
    int8_t value; // direction for ENCODER_ROTATION (1 / -1)
} encoder_event_t;

static bool IRAM_ATTR pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_wakeup;

    encoder_event_t event;
    event.type = ENCODER_ROTATION;
    if (edata->watch_point_value > 0) {
        event.value = 1;  // tick forward
    } else {
        event.value = -1; // tick backward
    }

    // clear count for calculate next tick
    pcnt_unit_clear_count(unit);

    xQueueSendFromISR(encoder_queue, &event, &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

static void IRAM_ATTR button_handler(void *arg) {
    int level = gpio_get_level(ENCODER_GPIO_BUTTON);
    encoder_event_t event;
    event.type = (level == 0) ? ENCODER_BUTTON_PRESSED : ENCODER_BUTTON_RELEASED;

    BaseType_t high_task_wakeup = pdFALSE;
    xQueueSendFromISR(encoder_queue, &event, &high_task_wakeup);
    if (high_task_wakeup == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void encoder_handler_task(void *arg) {
    encoder_event_t event;
    bool is_button_pressed = false;
    int64_t press_start_time = 0;

    while (1) {
        // dynamical timeout for check long press
        TickType_t wait_ticks = portMAX_DELAY;
        if (is_button_pressed) {
            int64_t elapsed = esp_timer_get_time() - press_start_time;
            int64_t remaining = LONG_PRESS_DELAY - elapsed;
            wait_ticks = (remaining > 0) ? pdMS_TO_TICKS(remaining / 1000) : 0;
        }

        if (xQueueReceive(encoder_queue, &event, wait_ticks) == pdTRUE) {
            int64_t now = esp_timer_get_time();

            switch (event.type) {
                case ENCODER_ROTATION:
                    if (s_rotation_callback != NULL) {
                        s_rotation_callback(event.value);
                    }
                    break;
                case ENCODER_BUTTON_PRESSED:
                    if (!is_button_pressed) {
                        if (gpio_get_level(ENCODER_GPIO_BUTTON) == 0) {
                            press_start_time = now;
                            is_button_pressed = true;
                        }
                    }
                    break;
                case ENCODER_BUTTON_RELEASED:
                    if (is_button_pressed) {
                        is_button_pressed = false;
                        int64_t duration = now - press_start_time;
                        if (duration >= BUTTON_DEBOUNCE && duration < LONG_PRESS_DELAY) {
                            if (s_short_press_callback != NULL) {
                                s_short_press_callback();
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        } else {
            // Queue timeout, button is long pressed, LONG_PRESS_DELAY timeout
            if (is_button_pressed) {
                is_button_pressed = false;
                if (s_long_press_callback != NULL) {
                    s_long_press_callback();
                }
            }
        }
    }
}

void encoder_button_setup(void) {
    const gpio_config_t btn_conf = {
          .pin_bit_mask = (1ULL << ENCODER_GPIO_BUTTON),
          .mode = GPIO_MODE_INPUT,
          .pull_up_en = GPIO_PULLUP_ENABLE,
          .pull_down_en = GPIO_PULLDOWN_DISABLE,
          .intr_type = GPIO_INTR_ANYEDGE // both rising and falling edge
      };
      ESP_ERROR_CHECK(gpio_config(&btn_conf));

      gpio_install_isr_service(0);
      gpio_isr_handler_add(ENCODER_GPIO_BUTTON, button_handler, NULL);
}

void encoder_pcnt_setup(void) {
    const pcnt_unit_config_t unit_config = {
        .high_limit = 100,
        .low_limit = -100,
        .intr_priority = 0,
        .flags = { .accum_count = 0 },
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    const pcnt_chan_config_t chan_config = {
        .edge_gpio_num = ENCODER_GPIO_A,
        .level_gpio_num = ENCODER_GPIO_B,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_channel));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel, 
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,  // when high act -> keep it as is
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE // when low act -> inverse value
    ));

    const pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = 1000 };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // configure watchpoints
    // add limit for 1 tick forward (2 steps) and backward (-2 steps)
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 2));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, -2));

    const pcnt_event_callbacks_t cbs = {
        .on_reach = pcnt_on_reach,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, encoder_queue));

    // run
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    ESP_LOGI(TAG, "Did setup PCNT X2 Quadrature Mode");
}

void encoder_setup(void) {
    encoder_queue = xQueueCreate(10, sizeof(encoder_event_t));

    encoder_pcnt_setup();
    encoder_button_setup();

    // create task for handle callbacks
    xTaskCreate(
        encoder_handler_task, 
        "encoder_task", 
        2048, 
        NULL, 
        10, 
        &encoder_task
    );
}

void encoder_register_rotation(encoder_rotation_callback_t callback) {
    s_rotation_callback = callback;
}

void encoder_register_short_press(encoder_button_callback_t callback) {
    s_short_press_callback = callback;
}

void encoder_register_long_press(encoder_button_callback_t callback) {
    s_long_press_callback = callback;
}

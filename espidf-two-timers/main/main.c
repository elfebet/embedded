#include <stdint.h>
#include <stdio.h>
#include "driver/gptimer.h"
#include "driver/gptimer_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hal/timer_types.h"
#include "portmacro.h"
#include "soc/clk_tree_defs.h"

#define LED1_PIN GPIO_NUM_4
#define LED2_PIN GPIO_NUM_5

#define FREQ1_HZ 1
#define FREQ2_HZ 3

static const char *TAG = "TWO_TIMERS";
static QueueHandle_t alarm_queue_1 = NULL;
static QueueHandle_t alarm_queue_2 = NULL;

static bool IRAM_ATTR on_timer_alarm(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx
) {
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    BaseType_t high_task_awoken = pdFALSE;
    uint32_t dummy = 0;
    xQueueSendFromISR(queue, &dummy, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

static void blink_task_1(void *arg) {
    uint32_t dummy = 0;
    bool led_state = false;
    while (1) {
        if (xQueueReceive(alarm_queue_1, &dummy, portMAX_DELAY)) {
            led_state = !led_state;
            gpio_set_level(LED1_PIN, led_state);
            ESP_LOGI(TAG, "Timer #1 (%d Hz) alarm! LED1 = %d", FREQ1_HZ, led_state);
        }
   }
}

static void blink_task_2(void *arg) {
    uint32_t dummy = 0;
    bool led_state = false;
    while (1) {
        if (xQueueReceive(alarm_queue_2, &dummy, portMAX_DELAY)) {
            led_state = !led_state;
            gpio_set_level(LED2_PIN, led_state);
            ESP_LOGI(TAG, "Timer #2 (%d Hz) alarm! LED2 = %d", FREQ2_HZ, led_state);
        }
    }
}

static gptimer_handle_t start_ctc_timer(uint32_t freq_hz, QueueHandle_t queue) {
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000,
    };

    gptimer_new_timer(&timer_config, &gptimer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = on_timer_alarm,
    };

    gptimer_register_event_callbacks(gptimer, &cbs, queue);

    gptimer_enable(gptimer);

    uint64_t alarm_count = 1000000 / freq_hz;

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = alarm_count,
        .flags.auto_reload_on_alarm = true,
    };

    gptimer_set_alarm_action(gptimer, &alarm_config);

    gptimer_start(gptimer);

    return gptimer;
}

void app_main() {
    gpio_config_t led_conf = {
        .pin_bit_mask = (1 << LED1_PIN | 1 << LED2_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };

    gpio_config(&led_conf);

    alarm_queue_1 = xQueueCreate(10, sizeof(uint32_t));
    alarm_queue_2 = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(blink_task_1, "blink_task_1", 2048, NULL, 10, NULL);
    xTaskCreate(blink_task_2, "blink_task_2", 2048, NULL, 10, NULL);

    start_ctc_timer(FREQ1_HZ, alarm_queue_1);
    start_ctc_timer(FREQ2_HZ, alarm_queue_2);
}
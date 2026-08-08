#include <stdbool.h>
#include <unistd.h>
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/pcnt_types.h"

#define ENCODER_GPIO_A 9
#define ENCODER_GPIO_B 10
#define PCNT_UNIT PCNT_UNIT_0

static const char *TAG = "Endoder";

pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_channel_0 = NULL;
pcnt_channel_handle_t pcnt_channel_1 = NULL;

QueueHandle_t pcnt_evt_queue = NULL;

// PCNT X4-mode
void setup_encoder_pcnt_x4() {
    const pcnt_unit_config_t unit_config = {
        .high_limit = 500,
        .low_limit = -500,
        .intr_priority = 0,
        .flags = {
            .accum_count = 1,
        },
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // === НАЛАШТУВАННЯ КАНАЛУ 0 ===
    const pcnt_chan_config_t chan_config_0 = {
        .edge_gpio_num = ENCODER_GPIO_A,
        .level_gpio_num = ENCODER_GPIO_B,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_0, &pcnt_channel_0));

    // При зростанні сигналу А: якщо В низький -> збільшуємо, якщо В високий -> зменшуємо
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_0, 
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,  // Негативний фронт (falling edge)
        PCNT_CHANNEL_EDGE_ACTION_INCREASE // Позитивний фронт (rising edge)
    ));
   
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_0,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,    // Коли рівень низький -> рахуємо як є
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE  // Коли рівень високий -> інвертуємо напрямок
    ));

    // === НАЛАШТУВАННЯ КАНАЛУ 1 ===
    const pcnt_chan_config_t chan_config_1 = {
        .edge_gpio_num = ENCODER_GPIO_B,
        .level_gpio_num = ENCODER_GPIO_A,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_1, &pcnt_channel_1));

    // Дзеркальна логіка для другого каналу, щоб вони не компенсували один одного
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_1,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE, // Негативний фронт інвертований порівняно з CH0
        PCNT_CHANNEL_EDGE_ACTION_DECREASE // Позитивний фронт інвертований порівняно з CH0
    ));
   
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_1,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    ESP_LOGI(TAG, "PCNT X4 Quadrature Mode Initialized");
}


// 1. Функція обробки події (викликається в контексті ISR переривання)
static bool IRAM_ATTR example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;

    // Надсилаємо значення точки спостереження, яка спрацювала, в чергу таска
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);

    return high_task_wakeup == pdTRUE;
}

// PCNT X2-mode
void setup_encoder_pcnt_x2() {
    const pcnt_unit_config_t unit_config = {
        .high_limit = 500,
        .low_limit = -500,
        .intr_priority = 0,
        .flags = {
            .accum_count = 1,
        },
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    const pcnt_chan_config_t chan_config = {
        .edge_gpio_num = ENCODER_GPIO_A,
        .level_gpio_num = ENCODER_GPIO_B,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_channel_0));

    // При зростанні сигналу А: якщо В низький -> збільшуємо, якщо В високий -> зменшуємо
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_0, 
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));
   
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_0,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE,    // Коли рівень низький -> рахуємо як є
        PCNT_CHANNEL_LEVEL_ACTION_KEEP  // Коли рівень високий -> інвертуємо напрямок
    ));


    const pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000, //1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // Створюємо чергу для передачі даних з ISR у звичайний таск
    pcnt_evt_queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 40));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, -40));
    const pcnt_event_callbacks_t cbs = {
        .on_reach = example_pcnt_on_reach,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, pcnt_evt_queue));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    ESP_LOGI(TAG, "PCNT X2 Quadrature Mode Initialized");
}

void encoder_monitor_task(void *arg) {
    int count = 0;
    while (1) {
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &count));
        ESP_LOGI(TAG, "Position: %d steps", count);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    setup_encoder_pcnt_x2();
    xTaskCreate(encoder_monitor_task, "encoder_task", 2048, NULL, 10, NULL);

    int triggered_wp = 0;
    while (1) {
        // Очікуємо повідомлення з черги про те, що лічильник досяг точки спостереження
        if (xQueueReceive(pcnt_evt_queue, &triggered_wp, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Подія! Досягнуто точку спостереження: %d", triggered_wp);

            int current_count = 0;
            pcnt_unit_get_count(pcnt_unit, &current_count);
            ESP_LOGI(TAG, "Поточне значення лічильника: %d", current_count);
        }
    }
}

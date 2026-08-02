#include "include/ldr.h"
#include <stdbool.h>
#include <unistd.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "config.h"

#define SMA_WINDOW_SIZE 10

static TaskHandle_t ldr_task_handle = NULL;
static const char *TAG = "LDR_task";

// buffer for SMA (Simple Moving Average)
static int sma_buffer[SMA_WINDOW_SIZE] = {0};
static int sma_index = 0;
static int sma_count = 0;
static int sma_sum = 0;

static int update_sma(int new_sample) {
    sma_sum -= sma_buffer[sma_index];
    sma_buffer[sma_index] = new_sample;
    sma_sum += new_sample;

    sma_index = (sma_index + 1) % SMA_WINDOW_SIZE;
    if (sma_count < SMA_WINDOW_SIZE) {
        sma_count++;
    }
    
    return sma_sum / sma_count;
}

static void ldrTask(void *arg) {
    // configure gpio for LEDs 
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LDR_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LDR_LED_PIN, 0);

    // init ADC oneshot
    adc_oneshot_unit_handle_t adc_handle;
    const adc_oneshot_unit_init_cfg_t init_config = { .unit_id = LDR_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // configure ADC channel
    const adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LDR_ADC_CHANNEL, &config));

    bool led_state = false;
    int64_t last_time = 0;

    while (1) {
        int raw_val = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, LDR_ADC_CHANNEL, &raw_val));

        // calculate AVG value
        const int filtered_val = update_sma(raw_val);

        // threshold logic
        if (filtered_val < LDR_THRESH_DARK && !led_state) {
            led_state = true;
            gpio_set_level(LDR_LED_PIN, 1);
            ESP_LOGI(TAG, "It got dark (%d) -> Turn on the LED", filtered_val);
        } 
        else if (filtered_val > LDR_THRESH_LIGHT && led_state) {
            led_state = false;
            gpio_set_level(LDR_LED_PIN, 0);
            ESP_LOGI(TAG, "It got light (%d) -> Turn off the LED", filtered_val);
        }

        const int64_t now = esp_timer_get_time();
        if (now - last_time > 1 * 1000 * 1000) {
            last_time = now;
            ESP_LOGI(TAG, "Raw: %d | Filtered: %d | LED: %d", raw_val, filtered_val, led_state);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void ldr_setup(void) {
    xTaskCreate(
        ldrTask, // function
        "LDR Task", // task name
        2048, // Stack size in bytes/words 
        NULL, // Parameters passed to task
        5, // Task Priority
        &ldr_task_handle // Task handle output
    );
}

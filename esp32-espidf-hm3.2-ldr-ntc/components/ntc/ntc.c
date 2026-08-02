#include "include/ntc.h"
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/adc_types.h"
#include "config.h"

// --- Параметри терморезистора та схеми ---
#define R_BALANCE           10000.0f  // Фіксований резистор (10k)
#define NTC_R0              10000.0f  // Опір NTC при 25°C (10k)
#define NTC_T0              298.15f   // 25°C у Кельвінах (25 + 273.15)
#define NTC_BETA            3950.0f   // Бета-коефіцієнт (типове значення 3950 K)
#define SMA_WINDOW_SIZE     10

static TaskHandle_t ntc_task_handle = NULL;
static const char *TAG = "NTC_task";

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

static float calculate_temperature(int raw_adc) {
    if (raw_adc <= 0 || raw_adc >= 4095) return -999.0f;

    // Для схеми, де NTC підключено до 3.3V, а фіксований резистор (R_BALANCE) до GND:
    // V_out = V_in * (R_BALANCE / (R_ntc + R_BALANCE))
    float r_ntc = R_BALANCE * ((ADC_MAX_VAL / (float)raw_adc) - 1.0f);

    // Обчислюємо температуру в Кельвінах за формулою Бета
    float steinhart = logf(r_ntc / NTC_R0) / NTC_BETA;
    steinhart += 1.0f / NTC_T0;
    float temp_kelvin = 1.0f / steinhart;

    // Переводимо в Цельсій
    return temp_kelvin - 273.15f;
}

static void ntcTask(void *arg) {
    // init ADC oneshot
    adc_oneshot_unit_handle_t adc_handle;
    const adc_oneshot_unit_init_cfg_t init_config = { .unit_id = NTC_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // configure ADC channel
    const adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, NTC_ADC_CHANNEL, &config));

    int64_t last_time = 0;
    while (1) {
        int raw_val = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, NTC_ADC_CHANNEL, &raw_val));
        
        const int filtered_val = update_sma(raw_val);
        const float temp_celsius = calculate_temperature(filtered_val);

        const int64_t now = esp_timer_get_time();
        if (now - last_time > 1 * 1000 * 1000) {
            last_time = now;
            ESP_LOGI(TAG, "Поточна температура: %.2f °C (ADC raw: %d)", temp_celsius, filtered_val);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void ntc_setup(void) {
    xTaskCreate(
        ntcTask, // function
        "NTC Task", // task name
        2048, // Stack size in bytes/words 
        NULL, // Parameters passed to task
        5, // Task Priority
        &ntc_task_handle // Task handle output
    );
}

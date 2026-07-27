#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <sys/_intsup.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hal/adc_types.h"
#include "esp_timer.h"

// configuration
#define ADC_UNIT           ADC_UNIT_1
#define ADC_CHANNEL        ADC_CHANNEL_5     // GPIO6
#define ADC_ATTEN          ADC_ATTEN_DB_12   // Повна шкала (0 - ~3.3V)
#define ADC_BITWIDTH       ADC_BITWIDTH_12   // 12bit resolution (0..4095)
#define VREF               3.3f              // Опорна напруга для формули (В)
#define MAX_RAW            4095.0f           // 2^12 - 1

static const char *TAG = "ADC_MEASURE";

// Ініціалізація схеми калібрування ESP-IDF
static bool init_adc_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    ESP_LOGI(TAG, "Try to init Curve Fitting calibration...");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH,
    };
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

    *out_handle = handle;
    return ret == ESP_OK;
}

static void print_header(void) {
    printf("\n==================================================================================\n");
    printf("  ADC: Vref = %.1f В | Bitwidth = 12-bit (0-4095) | Attenuation = 12dB\n", VREF);
    printf("==================================================================================\n");
    printf("%-6s | %-8s | %-14s | %-14s | %-12s\n", 
             "№", "RAW", "U_manual (mV)", "U_cali (mV)", "Error(%)");
    printf("----------------------------------------------------------------------------------\n");
}

void app_main(void) {
    // init ADC oneshot
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // configure channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    // init calibration
    adc_cali_handle_t adc_cali_handle = NULL;
    bool is_calibrated = init_adc_calibration(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN, &adc_cali_handle);
    if (is_calibrated) {
        ESP_LOGI(TAG, "ADC calibration connected!");
    } else {
        ESP_LOGW(TAG, "eFuse calibration data is not available.");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    print_header();

    int sample_count = 0;
    uint64_t start_time = esp_timer_get_time();

    while (1) {
        sample_count++;

        // read RAW data
        int raw_val = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_val));

        // calculated mV
        float v_calculated = ((float)raw_val / MAX_RAW) * VREF * 1000.0f;

        // calibrated mV
        float v_calibrated = 0.0f;
        if (is_calibrated) {
            int voltage_mv = 0;
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw_val, &voltage_mv));
            v_calibrated = voltage_mv;
        }

        // error in percent (%)
        float error_percent = 0.0f;
        if (v_calibrated > 0.001f) {
            error_percent = (fabsf(v_calculated - v_calibrated) / v_calibrated) * 100.0f;
        }

        // output row in table
        printf("%-6d | %-8d | %-14.3f | %-14.3f | %-12.2f%%\n",
               sample_count, raw_val, v_calculated, v_calibrated, error_percent);

        // repeat header every 40 measurements
        if (sample_count % 40 == 0) {
            print_header();
        }

        // finish script after 40 seconds
        uint64_t duration = esp_timer_get_time() - start_time;
        if (duration > 40 * 1000 * 1000) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    
    adc_oneshot_del_unit(adc1_handle);
    if (is_calibrated) {
        adc_cali_delete_scheme_curve_fitting(adc_cali_handle);
    }
    ESP_LOGI(TAG, "Script is finished");
}

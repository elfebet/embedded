#include <sys/_intsup.h>
#include "include/ldr.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_CHANNEL     ADC_CHANNEL_8   // Pin 9
#define ADC_UNIT        ADC_UNIT_1
#define ADC_ATTEN       ADC_ATTEN_DB_12 // 0..4095
#define ADC_BITWIDTH    ADC_BITWIDTH_12
#define SMA_WINDOW_SIZE 10

// buffer for SMA (Simple Moving Average)
static int sma_buffer[SMA_WINDOW_SIZE] = {0};
static int sma_index = 0;
static int sma_count = 0;
static int sma_sum = 0;

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t cali_handle;

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

void ldr_init() {
    const adc_oneshot_unit_init_cfg_t unit_config = { .unit_id = ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL,&channel_config));

    const adc_cali_curve_fitting_config_t cfg = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &cali_handle));
}

int ldr_raw_value() {
    int raw_val = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_val));
    return update_sma(raw_val);
}

int ldr_voltage_mv(int raw) {
    int mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));
    return mv;
}
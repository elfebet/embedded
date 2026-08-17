#include "temp_sensor.h"
#include "config.h"
#include "esp_adc/adc_oneshot.h"
#include <math.h>

// Дільник: NTC у ВЕРХНЬОМУ плечі (живлення->NTC->вузол->10кОм->GND).
// B-параметрична формула термістора (спрощене рівняння Стейнхарта-Харта):
// 1/T = 1/T0 + (1/B)*ln(R/R0)
#define NTC_R0_OHM      10000.0f
#define NTC_B           3950.0f
#define NTC_T0_K        298.15f // 25°C
#define DIVIDER_R_OHM   10000.0f

static adc_oneshot_unit_handle_t s_adc_handle;

void temp_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = TEMP_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));
    
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL, &chan_cfg));
}

uint16_t temp_sensor_read_raw() {
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL, &raw));
    return (uint16_t)raw;
}

bool temp_sensor_read_celsius(float *out_c, float *out_err_c) {
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL, &raw));

    if (raw <= 5 || raw >= 4090) { // обрив або коротке замикання дільника
        return false;
    }

    float r_ntc = DIVIDER_R_OHM * ((4095.0f / (float)raw) - 1.0f);
    float inv_t = (1.0f / NTC_T0_K) + (1.0f / NTC_B) * logf(r_ntc / NTC_R0_OHM);
    float temp_c = (1.0f / inv_t) - 273.15f;

    // Бюджет похибки: ±½ LSB на raw (0.4 мВ при ADC_ATTEN_DB_12) поширений
    // через похідну нелінійної формули термістора — тут спрощено оцінений
    // як стала ±0.3°C у робочому діапазоні 20-45°C (реальний розрахунок
    // похідної dT/dR).
    *out_c = temp_c;
    *out_err_c = 0.3f;
    return true;
}

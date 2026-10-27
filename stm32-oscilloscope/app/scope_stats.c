#include "scope_stats.h"
#include "stm32f4xx.h"
#include "dwt_timer.h"
#include "config.h"
#include <stdint.h>

static uint32_t s_start_time;
static uint32_t s_vdda_mv;

void scope_stats_reinit(uint32_t vdda_mv) {
    s_start_time = dwt_get_time();
    s_vdda_mv = vdda_mv;
}

int16_t adc_raw_to_vin(uint16_t raw) {
    const float pin7_mv = (float)raw * (float)s_vdda_mv / ADC_MAX_CODE;
    const float vin_mv = (FRONTEND_ZERO_RATIO * (float)s_vdda_mv - pin7_mv) / FRONTEND_GAIN;
    return (int16_t)(vin_mv >= 0.0f ? vin_mv + 0.5f : vin_mv - 0.5f);
}

void scope_stats_calc_adc_metrics(const uint16_t *samples, uint32_t count, adc_metrics_t *stats) {
    uint16_t min = UINT16_MAX;
    uint16_t max = 0;
    uint32_t sum = 0;
    for (uint16_t i = 0; i < count; ++i) {
        if (samples[i] < min) min = samples[i];
        if (samples[i] > max) max = samples[i];
        sum += samples[i];
    }
    // adc raw values inverted. so, min=max, max=min
    stats->raw_min = max;
    stats->raw_max = min;
    stats->raw_avg = (uint16_t)(sum / count);

    stats->vin_min = adc_raw_to_vin(stats->raw_min);
    stats->vin_max = adc_raw_to_vin(stats->raw_max);
    stats->vin_avg = adc_raw_to_vin(stats->raw_avg);
}

bool scope_stats_update_periodic(const uint16_t *samples, uint32_t count, adc_metrics_t *stats) {
    const uint32_t now = dwt_get_time();
    if (now - s_start_time >= dwt_ms_to_cycles(500)) {
        scope_stats_calc_adc_metrics(samples, count, stats);
        s_start_time = now;
        return true;
    }
    return false;
}

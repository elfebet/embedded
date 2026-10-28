#include "scope_adc_stats.h"
#include "stm32f4xx.h"
#include "dwt_timer.h"
#include "config.h"
#include <stdint.h>
#include <limits.h>

#define SIGNAL_MIN_AMPLITUDE_RAW 100U // ~0.3V at the input, below this the signal is treated as noise

static uint32_t s_start_time;
static uint32_t s_sample_rate_hz;
static float s_vdda_mv;

void scope_stats_reinit(uint32_t vdda_mv) {
    s_start_time = dwt_get_time();
    s_vdda_mv = (float)vdda_mv;
    s_sample_rate_hz = 0;
}

void scope_stats_set_sample_rate(uint32_t sample_rate_hz) {
    s_sample_rate_hz = sample_rate_hz;
}

float adc_raw_to_vm(uint16_t raw) {
    const float pin_mv = (float)raw * s_vdda_mv / ADC_MAX_CODE;
    return (FRONTEND_ZERO_RATIO * s_vdda_mv - pin_mv) / FRONTEND_GAIN;
}

int16_t adc_raw_to_vm_int(uint16_t raw) {
    const float mv = adc_raw_to_vm(raw);
    return (int16_t)(mv >= 0.0f ? mv + 0.5f : mv - 0.5f);
}

int16_t vm_to_adc_raw(uint16_t mv) {
    const float pin_mv = FRONTEND_ZERO_RATIO * s_vdda_mv - (float)mv * FRONTEND_GAIN;
    float raw = pin_mv * ADC_MAX_CODE / s_vdda_mv + 0.5f;
    if (raw < 0.0f) raw = 0.0f;
    if (raw > ADC_MAX_CODE) raw = ADC_MAX_CODE;
    return (uint16_t)raw;
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
    stats->raw_min = min;
    stats->raw_max = max;
    stats->raw_avg = (uint16_t)(sum / count);

    // adc raw values inverted. so, min=max, max=min
    stats->vin_min = adc_raw_to_vm_int(stats->raw_max);
    stats->vin_max = adc_raw_to_vm_int(stats->raw_min);
    stats->vin_avg = adc_raw_to_vm_int(stats->raw_avg);
}

void scope_stats_calc_signal_metrics(const uint16_t *samples, uint32_t count, const adc_metrics_t *stats, signal_metrics_t *metrics) {
    metrics->valid = false;
    if (count < 4 || s_sample_rate_hz == 0) return;

    const int32_t amplitude = stats->raw_max - stats->raw_min;
    if (amplitude < (int32_t)SIGNAL_MIN_AMPLITUDE_RAW) return;

    // The ADC is inverted, so the signal is mirrored: v grows with the input voltage.
    const int32_t flip = stats->raw_max + stats->raw_min;
    const float mid = (float)flip / 2.0f;
    const float hyst = (float)amplitude / 8.0f;

    bool armed_rise = false;
    bool armed_fall = false;
    bool has_rise = false;
    float last_rise = 0.0f;
    float first_rise = 0.0f;
    float high_sum = 0.0f;
    uint32_t high_count = 0;
    uint32_t rise_count = 0;

    float prev = (float)(flip - samples[0]);
    for (uint32_t i = 1; i < count; ++i) {
        const float v = (float)(flip - samples[i]);
        if (armed_rise && v >= mid && v != prev) {
            const float position = (float)(i - 1) + (mid - prev) / (v - prev);
            armed_rise = false;
            if (rise_count == 0) first_rise = position;
            last_rise = position;
            has_rise = true;
            ++rise_count;
        } else if (armed_fall && v <= mid && v != prev) {
            const float position = (float)(i - 1) + (prev - mid) / (prev - v);
            armed_fall = false;
            if (has_rise) {
                high_sum += position - last_rise;
                ++high_count;
                has_rise = false;
            }
        }

        // arm only after the edge check, so a sharp edge is not disarmed by its own sample
        if (v <= mid - hyst) {
            armed_rise = true;
            armed_fall = false;
        } else if (v >= mid + hyst) {
            armed_fall = true;
            armed_rise = false;
        }
        prev = v;
    }

    if (rise_count < 2 || high_count == 0) return;

    const float period_samples = (last_rise - first_rise) / (float)(rise_count - 1);
    if (period_samples <= 0.0f) return;

    float duty = high_sum / (float)high_count / period_samples * 100.0f;
    if (duty > 100.0f) duty = 100.0f;

    metrics->valid = true;
    metrics->period_us = period_samples * 1000000.0f / (float)s_sample_rate_hz;
    metrics->freq_hz = (float)s_sample_rate_hz / period_samples;
    metrics->duty_pct = duty;
}

void scope_stats_calc_metrics(const uint16_t *samples, uint32_t count, adc_metrics_t *adc, signal_metrics_t *signal) {
    if (samples == NULL || count == 0 || adc == NULL) return;

    scope_stats_calc_adc_metrics(samples, count, adc);
    if (signal != NULL) {
        scope_stats_calc_signal_metrics(samples, count, adc, signal);
    }
}

bool scope_stats_update_periodic(const uint16_t *samples, uint32_t count, adc_metrics_t *adc, signal_metrics_t *signal) {
    const uint32_t now = dwt_get_time();
    if (now - s_start_time >= dwt_ms_to_cycles(300)) {
        scope_stats_calc_metrics(samples, count, adc, signal);
        s_start_time = now;
        return true;
    }
    return false;
}

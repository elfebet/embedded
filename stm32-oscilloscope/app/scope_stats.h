#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t raw_min;
    uint16_t raw_max;
    uint16_t raw_avg;

    int16_t vin_min;
    int16_t vin_max;
    int16_t vin_avg;
} adc_metrics_t;

typedef struct {
    uint32_t freq_hz;
    uint32_t period_us;
    uint16_t duty_pct;
    bool valid;
} signal_metrics_t;

void scope_stats_reinit(uint32_t vdda_mv);
void scope_stats_set_sample_rate(uint32_t sample_rate_hz);

float adc_raw_to_vm(uint16_t raw); // must be called after scope_stats_reinit() to have a valid VDDA value
int16_t vm_to_adc_raw(uint16_t mv); // must be called after scope_stats_reinit() to have a valid VDDA value

void scope_stats_calc_metrics(const uint16_t *samples, uint32_t count, adc_metrics_t *adc, signal_metrics_t *signal);
bool scope_stats_update_periodic(const uint16_t *samples, uint32_t count, adc_metrics_t *adc, signal_metrics_t *signal);

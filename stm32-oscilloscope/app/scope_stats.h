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

void scope_stats_reinit(uint32_t vdda_mv);
void scope_stats_calc_adc_metrics(const uint16_t *samples, uint32_t count, adc_metrics_t *stats);
bool scope_stats_update_periodic(const uint16_t *samples, uint32_t count, adc_metrics_t *stats);

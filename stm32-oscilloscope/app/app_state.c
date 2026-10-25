#include "app_state.h"
#include "config.h"
#include <stdint.h>
#include <stdlib.h>

#define CALC_SAMPLE_RATE(time_div_us) ((ADC_SNAPSHOT_SIZE * 1000000ULL) / (time_div_us * (uint64_t)ADC_GRID_H_DIVS))
#define TIME_DIV_MODES_COUNT (sizeof(time_div_table) / sizeof(time_div_table[0]))

typedef struct {
    const char* label;
    uint32_t sample_rate_hz;
} time_div_t;

const time_div_t time_div_table[] = {
    { "200us",  CALC_SAMPLE_RATE(200) }, // 2 ms per frame
    { "500us",  CALC_SAMPLE_RATE(500) }, // 5 ms per frame
    { "1ms",    CALC_SAMPLE_RATE(1000) },
    { "2ms",    CALC_SAMPLE_RATE(2000) },
    { "5ms",    CALC_SAMPLE_RATE(5000) },
    { "10ms",   CALC_SAMPLE_RATE(10000) },
    { "20ms",   CALC_SAMPLE_RATE(20000) }, // 0.2 sec per frame
    { "50ms",   CALC_SAMPLE_RATE(50000) },
    { "100ms",  CALC_SAMPLE_RATE(100000) }, // 1 sec per frame
    { "200ms",  CALC_SAMPLE_RATE(200000) },
    { "500ms", CALC_SAMPLE_RATE(500000) },
    { "1sec",  CALC_SAMPLE_RATE(1000000) },
    { "2sec",  CALC_SAMPLE_RATE(2000000) },
    { "5sec",  CALC_SAMPLE_RATE(5000000) },
};

uint8_t find_time_div_index_by_sample_rate(uint32_t target_rate_hz) {
    uint8_t best_index = 0;
    uint32_t min_difference = 0xFFFFFFFF;
    for (uint8_t i = 0; i < TIME_DIV_MODES_COUNT; ++i) {
        uint32_t current_difference = (uint32_t)abs((int32_t)target_rate_hz - (int32_t)time_div_table[i].sample_rate_hz);
        if (current_difference < min_difference) {
            min_difference = current_difference;
            best_index = i;
        }
    }
    return best_index;
}
void app_state_init(app_state_t *state) {
    state->needs_update = 1;
    state->running = true;
    state->fps = 0;
    state->adc_vdda_mv = 0;
    state->display_mode = SCOPE_WAVE_DISPLAY_SAMPLE;

    state->time_div_index = find_time_div_index_by_sample_rate(ADC_PREFERRED_SAMPLE_RATE_HZ);
    state->sample_rate_hz = time_div_table[state->time_div_index].sample_rate_hz;

    state->vertical_knob_mode = VERTICAL_KNOB_VOLTS_DIV;
    state->volts_div_index = 0;
    state->offset_y = 0;
}

const char *app_state_vertical_knob_label(const app_state_t *state) {
    return state->vertical_knob_mode == VERTICAL_KNOB_OFFSET_Y ? "Y-pos" : "V/div";
}

const char *app_state_display_mode_label(const app_state_t *state) {
    switch (state->display_mode) {
    case SCOPE_WAVE_DISPLAY_SAMPLE:
        return "Sample";
    case SCOPE_WAVE_DISPLAY_PEAKS:
        return "Peaks";
    case SCOPE_WAVE_DISPLAY_HI_RES:
        return "Hi-Res";
    default:
        return "";
    }
}

static int32_t clamp_i32(int32_t value, int32_t min, int32_t max) {
    return value < min ? min : (value > max ? max : value);
}

uint8_t app_state_next_time_div(const app_state_t *state, int8_t delta) {
    uint8_t index = state->time_div_index;
    uint8_t next_index = (uint8_t)clamp_i32((int32_t)index + delta, 0, TIME_DIV_MODES_COUNT - 1);
    uint32_t sample_rate = time_div_table[next_index].sample_rate_hz;
    if (sample_rate >= ADC_MIN_SAMPLE_RATE_HZ && sample_rate <= ADC_MAX_SAMPLE_RATE_HZ) {
        return next_index;
    }
    return index;
}

const char *app_state_time_div_label(const app_state_t *state) {
    if (state->time_div_index < TIME_DIV_MODES_COUNT) {
        return time_div_table[state->time_div_index].label;
    }
    return "";
}

uint32_t app_state_time_div_sample_rate_hz(const app_state_t *state) {
    if (state->time_div_index < TIME_DIV_MODES_COUNT) {
        return time_div_table[state->time_div_index].sample_rate_hz;
    }
    return state->sample_rate_hz;
}

#include "app_state.h"
#include "config.h"

void app_state_init(app_state_t *state) {
    state->needs_update = 1;
    state->running = true;
    state->fps = 0;
    state->adc_vdda_mv = 0;
    state->sample_rate_hz = ADC_DEFAULT_SAMPLE_RATE_HZ;
    state->display_mode = SCOPE_WAVE_DISPLAY_SAMPLE;

    state->horizontal_knob_mode = HORIZONTAL_KNOB_TIME_DIV;
    state->vertical_knob_mode = VERTICAL_KNOB_VOLTS_DIV;
    state->time_div_index = 0;
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

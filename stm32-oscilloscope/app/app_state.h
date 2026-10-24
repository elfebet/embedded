#pragma once

typedef enum {
    SCOPE_WAVE_DISPLAY_SAMPLE, // Select one sample per horizontal pixel.
    SCOPE_WAVE_DISPLAY_PEAKS,  // Draw each bin's minimum-to-maximum range.
    SCOPE_WAVE_DISPLAY_HI_RES  // Average samples in each horizontal pixel bin.
} scope_wave_display_mode_t;

typedef struct {
    // current values
    uint8_t fps;
    uint32_t adc_vdda_mv;
    scope_wave_display_mode_t display_mode;

    // previous values
    uint8_t last_fps;
    scope_wave_display_mode_t last_display_mode;

    // flag indicating that all values need to be updated
    uint8_t needs_update;
} app_state_t;

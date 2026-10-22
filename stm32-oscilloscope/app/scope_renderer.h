#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SCOPE_GRID_H_DIVS  10U
#define SCOPE_GRID_V_DIVS  8U
#define SCOPE_GRID_WIDTH   280U
#define SCOPE_GRID_HEIGHT  160U

#define SCOPE_GRID_X      20U
#define SCOPE_GRID_Y      40U

typedef enum {
    SCOPE_WAVE_DISPLAY_SAMPLE, // Select one sample per horizontal pixel.
    SCOPE_WAVE_DISPLAY_PEAKS,  // Draw each bin's minimum-to-maximum range.
    SCOPE_WAVE_DISPLAY_HI_RES  // Average samples in each horizontal pixel bin.
} scope_wave_display_mode_t;

void scope_renderer_fill_background(void);
void scope_renderer_set_vdda_mv(uint32_t vdda_mv);
void scope_renderer_draw_grid(void); // must be called befor draw waves
void scope_renderer_draw_grid_test(void);

void scope_renderer_draw_wave(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode);
void scope_renderer_draw_wave_grouped(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode);

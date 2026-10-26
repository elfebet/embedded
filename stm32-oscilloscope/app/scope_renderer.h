#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "app_state.h"

#define SCOPE_GRID_H_DIVS  ADC_GRID_H_DIVS
#define SCOPE_GRID_V_DIVS  ADC_GRID_V_DIVS
#define SCOPE_GRID_WIDTH   280U
#define SCOPE_GRID_HEIGHT  160U

#define SCOPE_GRID_X      20U
#define SCOPE_GRID_Y      60U

void scope_renderer_reinit(uint32_t vdda_mv);

// draw waves
void scope_renderer_erase_wave(void);
void scope_renderer_draw_wave(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode);
void scope_renderer_erase_wave_grouped(void);
void scope_renderer_draw_wave_grouped(const uint16_t *samples, uint32_t sample_count, scope_wave_display_mode_t mode);

// update vertical div and offset
bool scope_renderer_update_vertical(const app_state_t *state); // true if the grid was reset and the wave must be redrawn

// draw labels
void scope_renderer_draw_run_stop(const app_state_t *state);
bool scope_renderer_draw_display_mode(const app_state_t *state); // true if mode changed and the wave must be redrawn
void scope_renderer_draw_time_div(const app_state_t *state);
void scope_renderer_draw_vertical(const app_state_t *state);
void scope_renderer_draw_sample_rate(const app_state_t *state);
void scope_renderer_draw_fps(const app_state_t *state);
bool scope_renderer_draw_waves_mode(const app_state_t *state); // true if mode changed and the wave must be redrawn

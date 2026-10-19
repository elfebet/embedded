#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SCOPE_GRID_H_DIVS  10U
#define SCOPE_GRID_V_DIVS  8U
#define SCOPE_GRID_WIDTH   280U
#define SCOPE_GRID_HEIGHT  160U

#define SCOPE_GRID_X      20U
#define SCOPE_GRID_Y      40U

void scope_renderer_fill_background(void);
void scope_renderer_draw_grid(void); // must be called befor draw waves
void scope_renderer_draw_grid_test(void);

void scope_renderer_draw_wave(const uint16_t *samples);
void scope_renderer_draw_wave_grouped(const uint16_t *samples, uint32_t sample_count);

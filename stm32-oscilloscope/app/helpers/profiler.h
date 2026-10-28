#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t avg_wave_render_us;
    uint8_t fps;
} profiler_stats_t;

void profiler_init(void);
void profiler_frame_end(void);

void profiler_wave_render_begin(void);
void profiler_wave_render_end(void);

bool profiler_take_stats(profiler_stats_t *stats);

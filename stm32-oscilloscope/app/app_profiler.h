#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t avg_wave_render_us;
    uint8_t fps;
} app_profiler_stats_t;

void app_profiler_init(void);
void app_profiler_frame_end(void);

void app_profiler_wave_render_begin(void);
void app_profiler_wave_render_end(void);

bool app_profiler_take_stats(app_profiler_stats_t *stats);

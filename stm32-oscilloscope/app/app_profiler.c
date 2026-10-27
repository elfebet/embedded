#include "app_profiler.h"
#include "stm32f4xx.h"
#include "dwt_timer.h"

static uint32_t s_frames_in_window;
static uint32_t s_window_start_cycles;

static uint32_t s_wave_render_start;
static uint64_t s_wave_render_cycles;
static uint32_t s_wave_render_count;

static app_profiler_stats_t s_stats;
static bool s_stats_ready;

void app_profiler_init(void) {
    s_frames_in_window = 0;
    s_window_start_cycles = dwt_get_time();

    s_wave_render_start = 0;
    s_wave_render_cycles = 0;
    s_wave_render_count = 0;

    s_stats_ready = false;
}

void app_profiler_wave_render_begin(void) {
    s_wave_render_start = dwt_get_time();
}

void app_profiler_wave_render_end(void) {
    s_wave_render_cycles += dwt_get_time() - s_wave_render_start;
    s_wave_render_count++;
}

void app_profiler_frame_end(void) {
    s_frames_in_window++;

    const uint32_t now = dwt_get_time();
    const uint32_t elapsed_cycles = now - s_window_start_cycles;
    if (elapsed_cycles >= SystemCoreClock) {
        s_stats.fps = (uint8_t)(((uint64_t)s_frames_in_window * SystemCoreClock + elapsed_cycles / 2) / elapsed_cycles);
        s_stats.avg_wave_render_us = s_wave_render_count ? dwt_cycles_to_us(s_wave_render_cycles / s_wave_render_count) : 0;

        s_window_start_cycles = now;
        s_frames_in_window = 0;

        s_wave_render_cycles = 0;
        s_wave_render_count = 0;
        s_stats_ready = true;
    }
}

bool app_profiler_take_stats(app_profiler_stats_t *stats) {
    if (stats == NULL || !s_stats_ready) return false;

    *stats = s_stats;
    s_stats_ready = false;
    return true;
}

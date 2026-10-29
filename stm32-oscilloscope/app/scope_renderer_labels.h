#pragma once
#include "app_state.h"
#include "scope_stats.h"
#include "scope_capture.h"

void scope_renderer_labels_clear_buffer(void);

void scope_renderer_draw_run_stop(const app_state_t *state);
void scope_renderer_draw_display_mode(const app_state_t *state);
void scope_renderer_draw_vertical(const app_state_t *state); // draw V/div, Vpos, active encoder action
void scope_renderer_draw_time_div(const app_state_t *state);

void scope_renderer_draw_trigger_mode(const app_state_t *state);
void scope_renderer_draw_trigger_edge(const app_state_t *state);
void scope_renderer_draw_trigger_position(const app_state_t *state);
void scope_renderer_draw_trigger_level(const app_state_t *state);

void scope_renderer_draw_signal_metrics(const signal_metrics_t *metrics); // frequency, period, duty under the graph
void scope_renderer_draw_sample_rate(const app_state_t *state);
void scope_renderer_draw_adc_vdda(const adc_metrics_t *metrics);

void scope_renderer_draw_waves_mode(const app_state_t *state);
void scope_renderer_draw_capture_state(scope_capture_state_t state);
void scope_renderer_draw_fps(uint8_t fps);

// helper functions for drawing labels on the scope display
void scope_renderer_draw_top_labels(const app_state_t *state);
void scope_renderer_draw_bottom_labels(const adc_metrics_t *adc, const signal_metrics_t *signal);

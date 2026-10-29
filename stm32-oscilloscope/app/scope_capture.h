#pragma once
#include "app_state.h"
#include <stdint.h>

typedef enum {
    SCOPE_CAPTURE_STATE_ROLL = 0,
    SCOPE_CAPTURE_STATE_WAIT,  // armed, waiting for the trigger event
    SCOPE_CAPTURE_STATE_TRIGD, // the displayed frame was captured by a trigger event
    SCOPE_CAPTURE_STATE_AUTO,  // auto timeout, free running without sync
} scope_capture_state_t;

typedef struct {
    const uint16_t *samples;
    uint32_t samples_count;
    scope_capture_state_t state;
    bool single_completed_update;
    bool new_frame_ready;
} scope_capture_result_t;

void scope_capture_init(const uint16_t *scan_samples);
void scope_capture_update(scope_capture_result_t *result, const app_state_t *state, uint32_t total_samples);

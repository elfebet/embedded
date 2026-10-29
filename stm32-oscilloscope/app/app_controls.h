#pragma once
#include "input_events.h"
#include "scope_renderer.h"
#include "app_state.h"

// true - action applied, false - no action applied
bool app_controls_handle(app_state_t *state, const input_events_t *events);

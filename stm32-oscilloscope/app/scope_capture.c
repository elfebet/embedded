#include "scope_capture.h"
#include "app_state.h"
#include "config.h"
#include "dwt_timer.h"
#include "scope_stats.h"
#include <stdbool.h>
#include <string.h>

#define AUTO_TIMEOUT_MIN_MS    100U
#define AUTO_TIMEOUT_MAX_MS    2000U
#define TRIGGER_HYSTERESIS_RAW 30 // ~0.1V at the input, rejects noise chatter around the level

typedef struct {
    const uint16_t *scan_samples;
    uint32_t trigger_wait_started_at;
    uint32_t single_arm_total;

    // values to detect changes in the app state to reset capture logic if needed
    uint32_t sample_rate_hz;
    trigger_mode_t last_trigger_mode;
    trigger_edge_t last_trigger_edge;
    int32_t last_trigger_level_mv;
    bool was_running;
    scope_capture_state_t state;
} scope_capture_data_t;

static scope_capture_data_t s_capture;
static uint16_t adc_snapshot[ADC_SNAPSHOT_SIZE];

static void set_capture_state(scope_capture_result_t *result, scope_capture_state_t state) {
    s_capture.state = state;
    result->state = state;
}

static bool is_timeout(uint32_t time) {
    // keep the last stable frame until the timeout
    uint32_t timeout_ms = 2000000U / s_capture.sample_rate_hz; // two frame durations
    if (timeout_ms < AUTO_TIMEOUT_MIN_MS) timeout_ms = AUTO_TIMEOUT_MIN_MS;
    if (timeout_ms > AUTO_TIMEOUT_MAX_MS) timeout_ms = AUTO_TIMEOUT_MAX_MS;
    return time - s_capture.trigger_wait_started_at >= dwt_ms_to_cycles(timeout_ms);
}

// Finds a trigger crossing in samples[min_index..max_index]. The ADC is inverted:
// a rising input voltage means a falling raw code. The crossing must be preceded by
// a sample beyond the hysteresis band on the opposite side.
static bool find_trigger(const app_state_t *state, uint32_t min_index, uint32_t max_index, uint32_t *found_index) {
    const bool rising = state->trigger_edge == TRIGGER_EDGE_RISING;
    const bool want_first = state->trigger_mode == TRIGGER_MODE_SINGLE;
    const uint16_t level = vm_to_adc_raw(state->trigger_level_mv);
    bool armed = false;
    bool found = false;

    for (uint32_t i = 0; i < ADC_TRIGGER_SCAN_SIZE; ++i) {
        const int32_t sample = s_capture.scan_samples[i];
        bool crossed = false;
        if (rising) {
            if (sample > (int32_t)level + TRIGGER_HYSTERESIS_RAW)
                armed = true;
            else if (armed && sample <= level)
                crossed = true;
        } else {
            if (sample < (int32_t)level - TRIGGER_HYSTERESIS_RAW)
                armed = true;
            else if (armed && sample >= level)
                crossed = true;
        }
        if (!crossed) continue;

        armed = false;
        if (i < min_index || i > max_index) continue;
        *found_index = i;
        found = true;
        if (want_first) break;
    }

    return found;
}

void scope_capture_init(const uint16_t *scan_samples) {
    s_capture.scan_samples = scan_samples;
    s_capture.trigger_wait_started_at = 0;
    s_capture.single_arm_total = 0;
    s_capture.last_trigger_mode = TRIGGER_MODE_ROLL;
    s_capture.last_trigger_edge = TRIGGER_EDGE_RISING;
    s_capture.last_trigger_level_mv = 0;
    s_capture.was_running = false;
    s_capture.sample_rate_hz = 0;
    s_capture.state = SCOPE_CAPTURE_STATE_ROLL;
    const int16_t zero_raw = vm_to_adc_raw(0);
    for (uint32_t i = 0; i < ADC_SNAPSHOT_SIZE; ++i) {
        adc_snapshot[i] = zero_raw;
    }
}

static void update_capture_tracking(const app_state_t *state, uint32_t total_samples, uint32_t now) {
    const bool single_active = state->running && state->trigger_mode == TRIGGER_MODE_SINGLE;
    const bool was_single_active = s_capture.was_running && s_capture.last_trigger_mode == TRIGGER_MODE_SINGLE;
    const bool trigger_config_changed =
        state->trigger_edge != s_capture.last_trigger_edge ||
        state->trigger_level_mv != s_capture.last_trigger_level_mv;
    const bool capture_conditions_changed =
        state->running != s_capture.was_running ||
        state->trigger_mode != s_capture.last_trigger_mode ||
        state->sample_rate_hz != s_capture.sample_rate_hz ||
        trigger_config_changed;

    if (capture_conditions_changed) {
        s_capture.trigger_wait_started_at = now;
    }

    if (single_active &&
        (!was_single_active || state->sample_rate_hz != s_capture.sample_rate_hz || trigger_config_changed)) {
        s_capture.single_arm_total = total_samples;
    }

    if (state->sample_rate_hz != s_capture.sample_rate_hz) {
        s_capture.sample_rate_hz = state->sample_rate_hz;
    }
    if (state->trigger_mode != s_capture.last_trigger_mode) {
        s_capture.last_trigger_mode = state->trigger_mode;
    }
    if (state->trigger_edge != s_capture.last_trigger_edge) {
        s_capture.last_trigger_edge = state->trigger_edge;
    }
    if (state->trigger_level_mv != s_capture.last_trigger_level_mv) {
        s_capture.last_trigger_level_mv = state->trigger_level_mv;
    }
    if (state->running != s_capture.was_running) {
        s_capture.was_running = state->running;
    }
}

void scope_capture_update(scope_capture_result_t *result, const app_state_t *state, uint32_t total_samples) {
    if (result == NULL) return;

    result->state = s_capture.state;
    result->samples.data = adc_snapshot;
    result->samples.count = ADC_SNAPSHOT_SIZE;
    result->single_completed_update = false;
    result->new_frame_ready = false;
    if (state == NULL) return;

    const uint32_t now = dwt_get_time();
    update_capture_tracking(state, total_samples, now);

    if (!state->running) return;

    if (state->trigger_mode == TRIGGER_MODE_ROLL) {
        memcpy(adc_snapshot, &s_capture.scan_samples[ADC_TRIGGER_SCAN_SIZE - ADC_SNAPSHOT_SIZE], sizeof(adc_snapshot));
        set_capture_state(result, SCOPE_CAPTURE_STATE_ROLL);
        result->new_frame_ready = true;
        return;
    }

    const uint32_t pre_samples = ADC_SNAPSHOT_SIZE * app_state_trigger_position_percent(state) / 100;
    const uint32_t post_samples = ADC_SNAPSHOT_SIZE - pre_samples;
    const uint32_t valid = total_samples < ADC_TRIGGER_SCAN_SIZE ? total_samples : ADC_TRIGGER_SCAN_SIZE;
    uint32_t min_index = ADC_TRIGGER_SCAN_SIZE - valid + pre_samples;
    const uint32_t max_index = ADC_TRIGGER_SCAN_SIZE - post_samples;

    const bool single_mode = state->trigger_mode == TRIGGER_MODE_SINGLE;
    if (single_mode) {
        // only events that happened after arming
        uint32_t since_arm = total_samples - s_capture.single_arm_total;
        if (since_arm > ADC_TRIGGER_SCAN_SIZE) {
            since_arm = ADC_TRIGGER_SCAN_SIZE;
        }
        if (ADC_TRIGGER_SCAN_SIZE - since_arm > min_index) {
            min_index = ADC_TRIGGER_SCAN_SIZE - since_arm;
        }
    }
    uint32_t trigger_index = 0;
    const bool trigger_found = find_trigger(state, min_index, max_index, &trigger_index);
    if (trigger_found) {
        memcpy(adc_snapshot, &s_capture.scan_samples[trigger_index - pre_samples], sizeof(adc_snapshot));
        s_capture.trigger_wait_started_at = now;
        result->single_completed_update = single_mode;
        set_capture_state(result, SCOPE_CAPTURE_STATE_TRIGD);
        result->new_frame_ready = true;
        return;
    }

    if (single_mode) {
        set_capture_state(result, SCOPE_CAPTURE_STATE_WAIT);
    } else if (is_timeout(now)) {
        if (state->trigger_mode == TRIGGER_MODE_NORMAL) {
            set_capture_state(result, SCOPE_CAPTURE_STATE_WAIT);

        } else if (state->trigger_mode == TRIGGER_MODE_AUTO) {
            memcpy(adc_snapshot, &s_capture.scan_samples[ADC_TRIGGER_SCAN_SIZE - ADC_SNAPSHOT_SIZE], sizeof(adc_snapshot));
            set_capture_state(result, SCOPE_CAPTURE_STATE_AUTO);
            result->new_frame_ready = true;
        }
    }
}

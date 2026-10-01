#include "motor_fsm.h"
#include <stddef.h>

typedef void (*state_handler_t)(motor_fsm_t *fsm, motor_event_t event, uint32_t now);

// helpers
static void enter(motor_fsm_t *fsm, motor_state_t state) {
    if (fsm->state == state) return;

    fsm->state = state;
    if (fsm->on_state) {
        fsm->on_state(fsm->cb_ctx, state);
    }
}

static void stop(motor_fsm_t *fsm, uint32_t now) {
    motor_stop(&fsm->motor);
    fsm->time_stopped = now;
}

static void start(motor_fsm_t *fsm, motor_direction_t dir) {
    motor_run(&fsm->motor, dir);
    enter(fsm, dir == MOTOR_DIR_CW ? MS_RUN_CW : MS_RUN_CCW);
}

static bool is_step(motor_event_t event, motor_direction_t *dir) {
    if (event == MEV_STEP_CW) {
        *dir = MOTOR_DIR_CW;
        return true;
    }
    if (event == MEV_STEP_CCW) {
        *dir = MOTOR_DIR_CCW;
        return true;
    }
    return false;
}

static void request_direction(motor_fsm_t *fsm, motor_direction_t dir, uint32_t now) {
    fsm->time_last_step = now;
    if ((now - fsm->time_stopped) >= fsm->config.deadtime_ms) {
        start(fsm, dir);
    } else {
        fsm->target = dir;
        enter(fsm, MS_SWITCHING);
    }
}

static void state_idle(motor_fsm_t *fsm, motor_event_t event, uint32_t now) {
    motor_direction_t dir;
    if (is_step(event, &dir)) {
        request_direction(fsm, dir, now);
    }
}

static void state_running(motor_fsm_t *fsm, motor_event_t event, uint32_t now) {
    motor_direction_t cur = (fsm->state == MS_RUN_CW) ? MOTOR_DIR_CW : MOTOR_DIR_CCW;
    motor_direction_t dir;

    if (is_step(event, &dir)) {
        if (dir == cur) {
            fsm->time_last_step = now;
        } else {
            stop(fsm, now);
            fsm->time_last_step = now;
            fsm->target = dir;
            enter(fsm, MS_SWITCHING);
        }
        return;
    }

    if (event == MEV_STOP) {
        stop(fsm, now);
        enter(fsm, MS_IDLE);
        return;
    }

    uint32_t idle_timeout = fsm->config.idle_timeout_ms;
    if (event == MEV_TICK && idle_timeout > 0 && (now - fsm->time_last_step) >= idle_timeout) {
        stop(fsm, now);
        enter(fsm, MS_IDLE);
    }
}

static void state_switching(motor_fsm_t *fsm, motor_event_t event, uint32_t now) {
    motor_direction_t dir;

    if (is_step(event, &dir)) {
        fsm->target = dir;
        fsm->time_last_step = now;
        return;
    }

    if (event == MEV_STOP) {
        enter(fsm, MS_IDLE);
        return;
    }

    if (event == MEV_TICK && (now - fsm->time_stopped) >= fsm->config.deadtime_ms) {
        start(fsm, fsm->target);
    }
}

// TABLE OF STATES
static const state_handler_t STATE_HANDLERS[MS_COUNT] = {
    [MS_IDLE]      = state_idle,
    [MS_RUN_CW]    = state_running,
    [MS_RUN_CCW]   = state_running,
    [MS_SWITCHING] = state_switching,
};

void motor_fsm_init(
    motor_fsm_t *fsm,
    const motor_fsm_config_t *cfg,
    motor_t motor,
    MotorStateCallback_t on_state,
    void *cb_ctx,
    uint32_t now_ms
) {
    fsm->config = *cfg;
    fsm->motor = motor;
    fsm->on_state = on_state;
    fsm->cb_ctx = cb_ctx;
    fsm->state = MS_IDLE;
    fsm->target = MOTOR_DIR_CW;
    fsm->time_last_step = now_ms;
    fsm->time_stopped = now_ms - cfg->deadtime_ms;

    motor_stop(&fsm->motor);
}

void motor_fsm_dispatch(motor_fsm_t *fsm, motor_event_t event, uint32_t now_ms) {
    if (fsm->state < MS_COUNT && STATE_HANDLERS[fsm->state] != NULL) {
        STATE_HANDLERS[fsm->state](fsm, event, now_ms);
    }
}

motor_state_t motor_fsm_state(const motor_fsm_t *fsm) {
    return fsm->state;
}

const char *motor_fsm_state_name(motor_state_t state) {
    static const char *const NAMES[MS_COUNT] = { "IDLE", "RUN_CW", "RUN_CCW", "SWITCHING" };
    return (state < MS_COUNT) ? NAMES[state] : "?";
}

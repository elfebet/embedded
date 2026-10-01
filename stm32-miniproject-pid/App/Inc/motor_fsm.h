#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MOTOR_DIR_CW = 0,
    MOTOR_DIR_CCW
} motor_direction_t;

// table of operations (vtable)
typedef struct {
    void (*run) (void *self, motor_direction_t dir);
    void (*stop)(void *self);
} motor_ops_t;

// abstract motor
typedef struct {
    const motor_ops_t *ops;
    void *self;
} motor_t;

// helpers
static inline void motor_run(const motor_t *m, motor_direction_t dir) { m->ops->run(m->self, dir); }
static inline void motor_stop(const motor_t *m) { m->ops->stop(m->self); }

typedef enum {
    MS_IDLE = 0,
    MS_RUN_CW,
    MS_RUN_CCW,
    MS_SWITCHING,
    MS_COUNT
} motor_state_t;

typedef enum {
    MEV_STEP_CW = 0,
    MEV_STEP_CCW,
    MEV_STOP,
    MEV_TICK
} motor_event_t;

typedef void (*MotorStateCallback_t)(void *ctx, motor_state_t new_state);

typedef struct {
    uint32_t deadtime_ms;      // pause between directions
    uint32_t idle_timeout_ms;  // delay for stop when the encoder is inactive, 0 = non-stop
} motor_fsm_config_t;

typedef struct {
    motor_state_t state;
    motor_direction_t target;
    uint32_t time_last_step; // ms
    uint32_t time_stopped;   // ms
    motor_fsm_config_t config;
    motor_t motor;
    MotorStateCallback_t on_state; // observer
    void *cb_ctx;
} motor_fsm_t;

void motor_fsm_init(
    motor_fsm_t *fsm,
    const motor_fsm_config_t *cfg,
    motor_t motor,
    MotorStateCallback_t on_state,
    void *cb_ctx,
    uint32_t now_ms
);

void motor_fsm_dispatch(motor_fsm_t *fms, motor_event_t event, uint32_t now_ms);
motor_state_t motor_fsm_state(const motor_fsm_t *fms);
const char *motor_fsm_state_name(motor_state_t state);

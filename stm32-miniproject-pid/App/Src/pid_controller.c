/* ============================================================================
 * Example
 *   setpoint 1.6 V, measurement 1.5 V, prev_meas 1.49 В, integ = 0.45, diff = 0
 *     err   = 1,6 − 1,5                   = 0,10
 *     P     = 2,0 × 0,10                  = 0,20   (pushes forward)
 *     D     = 0,6 × 0 − 8 × (1,50 − 1,49) = −0,08  (braking: the tension is already rising)
 *     di    = 3,0 × 0,10 × 0,01           = 0,003
 *     integ = 0,45 + 0,003                = 0,453
 *     u     = 0,20 + 0,453 − 0,08         = 0,573  -> PWM 57.3 %
 * ========================================================================== */
#include "pid_controller.h"

static inline float clampf(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

void pid_init(pid_data_t *pid, float kp, float ki, float kd, float ts, float tau, float out_min, float out_max) {
    pid->ts = ts;           // 0.01 s
    pid->tau = tau;         // 0.02 s
    pid->out_min = out_min; // 0
    pid->out_max = out_max; // 1
    pid_set_gains(pid, kp, ki, kd);
    pid_reset(pid);
}

void pid_set_gains(pid_data_t *pid, float kp, float ki, float kd) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void pid_reset(pid_data_t *pid) {
    pid->integ = 0.0f;
    pid->diff = 0.0f;
    pid->prev_meas = 0.0f;
    pid->first = true;
    pid->p_term = 0.0f;
    pid->i_term = 0.0f;
    pid->d_term = 0.0f;
}

float pid_update(pid_data_t *pid, float setpoint, float measurement) {
    if (pid->first) {
        pid->prev_meas = measurement;
        pid->first = false;
    }
    const float err = setpoint - measurement;

    // P
    const float p = pid->kp * err;

    // D
    const float a = (2.0f * pid->tau - pid->ts) / (2.0f * pid->tau + pid->ts); // = 0.6
    const float b = (2.0f * pid->kd) / (2.0f * pid->tau + pid->ts); // = 8.0
    pid->diff = a * pid->diff - b * (measurement - pid->prev_meas);
    pid->prev_meas = measurement;

    // I
    const float di = pid->ki * err * pid->ts;
    const float u_unsat = p + pid->integ + di + pid->diff;
    const bool sat_hi = (u_unsat > pid->out_max) && (err > 0.0f);
    const bool sat_lo = (u_unsat < pid->out_min) && (err < 0.0f);
    if (!sat_hi && !sat_lo) {
        pid->integ += di;
    }
    pid->integ = clampf(pid->integ, pid->out_min, pid->out_max);

    // values for debug
    pid->p_term = p;
    pid->i_term = pid->integ;
    pid->d_term = pid->diff;

    const float u = clampf(p + pid->integ + pid->diff, pid->out_min, pid->out_max);
    return u;
}

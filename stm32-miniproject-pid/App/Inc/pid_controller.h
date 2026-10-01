#pragma once
#include <stdbool.h>

typedef struct {
    // params
    float kp, ki, kd;         // coefficients: P, I, D
    float ts;                 // період дискретизації, с (0,01)
    float tau;                // стала часу фільтра D-складової, с (0,02; має бути ≥ 2·ts)
    float out_min, out_max;   // 0..1 = 0..100 % PWM

    // stored values, PID values between calls
    float integ;     // accumulated I (already multiplied by Ki)
    float diff;      // filtered D
    float prev_meas; // previous meas (for D)
    bool  first;     // first call?

    // for debug
    float p_term, i_term, d_term;
} pid_data_t;

void pid_init(pid_data_t *pid, float kp, float ki, float kd, float ts, float tau, float out_min, float out_max);
void pid_set_gains(pid_data_t *pid, float kp, float ki, float kd);
void pid_reset(pid_data_t *pid);

float pid_update(pid_data_t *pid, float setpoint, float measurement); // one step -> 0..1

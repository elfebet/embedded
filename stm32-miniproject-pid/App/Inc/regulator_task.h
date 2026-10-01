#pragma once
#include "drv_pwm.h"
#include "drv_adc.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t  setpoint_mv;
    int32_t  measured_mv;
    int32_t  error_mv;

    float    duty_pct; // pwm, %
    float    p, i, d;  // PID components (0..1)

    bool     settled;     // вийшов на уставку
    uint32_t exec_us;     // скільки мкс зайняв останній такт
    uint32_t exec_us_max; // найдовший такт
    uint32_t overruns;    // скільки разів не встиг у свій період 10 мс
} reg_status_t;

typedef struct {
    float kp, ki, kd; // coefficients that can be changed in debug
} pid_tuning_t;

extern volatile reg_status_t g_reg;
extern volatile pid_tuning_t g_pid_tuning;  // readonly / can be changed in debug

void regulator_task_init(const pwm_t *pwm, const adc_t *adc, unsigned long taskPriority);

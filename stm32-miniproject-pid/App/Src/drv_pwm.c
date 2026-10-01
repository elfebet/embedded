#include "drv_pwm.h"

void pwm_start(const pwm_t *p) {
    __HAL_TIM_SET_COMPARE(p->htim, p->channel, 0); // CCR = 0 -> 0% before start
    HAL_TIM_PWM_Start(p->htim, p->channel);
}

void pwm_set_duty(const pwm_t *p, float duty) {
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    uint32_t period = __HAL_TIM_GET_AUTORELOAD(p->htim) + 1u; // period from timer: ARR+1 = 4200.
    __HAL_TIM_SET_COMPARE(p->htim, p->channel, (uint32_t)(duty * (float)period));
}

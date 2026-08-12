#include "include/servo.h"
#include <driver/ledc.h>

#define SERVO_PIN                    14
#define SERVO_FREQ                   50
#define SERVO_TIMEBASE_PERIOD_US     20000   // (1000000 us / 50 Hz)
#define SERVO_MIN_PULSE_US           500     // min pulse width, 0°
#define SERVO_MAX_PULSE_US           2400    // max pulse width, 180°
#define SERVO_MAX_ANGLE              180 

#define LEDC_TIMER_NUM   LEDC_TIMER_0
#define LEDC_CHANNEL_NUM LEDC_CHANNEL_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES    LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY   ((1 << LEDC_DUTY_RES) - 1)

static int s_servo_angle = 0;

void servo_init() {
    const ledc_timer_config_t timer_cfg = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = SERVO_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    const ledc_channel_config_t channel_cfg = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_NUM,
        .timer_sel      = LEDC_TIMER_NUM,
        .gpio_num       = SERVO_PIN,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

static uint32_t angle_to_pulse(int angle) {
    if (angle <= 0)
        return SERVO_MIN_PULSE_US;
    else if (angle >= SERVO_MAX_ANGLE)
        return SERVO_MAX_PULSE_US;
    else 
        return SERVO_MIN_PULSE_US + (angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / SERVO_MAX_ANGLE;
}

void servo_set_angle(int angle) {
    if (s_servo_angle == angle) return;

    s_servo_angle = angle;
    uint32_t pulse = angle_to_pulse(angle);
    uint32_t duty = (pulse * SERVO_MAX_DUTY) / SERVO_TIMEBASE_PERIOD_US;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_NUM, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_NUM));
}

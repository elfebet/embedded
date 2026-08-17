#include "clamp_servo.h"
#include "driver/ledc.h"

#define SERVO_PIN       15
#define SERVO_TIMER     LEDC_TIMER_1
#define SERVO_MODE      LEDC_LOW_SPEED_MODE
#define SERVO_CHANNEL   LEDC_CHANNEL_1
#define SERVO_FREQ_HZ   50
#define SERVO_RES       LEDC_TIMER_14_BIT

#define SERVO_MIN_US    500.0f // 0% opened (closed)
#define SERVO_MAX_US    2500.0f // 100% opened (clamp fully opened)
#define SERVO_PERIOD_US 20000.0f

static void servo_write_pulse(float pulse_us) {
    uint32_t max_duty = (1u << SERVO_RES) - 1u;
    uint32_t duty = (uint32_t)((pulse_us / SERVO_PERIOD_US) * max_duty);
    ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(SERVO_MODE, SERVO_CHANNEL);
}

void clamp_servo_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = SERVO_MODE,
        .duty_resolution = SERVO_RES,
        .timer_num = SERVO_TIMER,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .gpio_num = SERVO_PIN,
        .speed_mode = SERVO_MODE,
        .channel = SERVO_CHANNEL,
        .timer_sel = SERVO_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);

    servo_write_pulse(SERVO_MIN_US); // safe state by default: closed
}

void clamp_servo_set_open_percent(uint8_t percent) {
    if (percent > 100) percent = 100;

    float pulse_us = SERVO_MIN_US + (percent / 100.0f) * (SERVO_MAX_US - SERVO_MIN_US);
    servo_write_pulse(pulse_us);
}
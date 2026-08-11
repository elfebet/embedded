#include "include/servo_control.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

// Congifure for SG90 (Micro Servo)
#define SERVO_FREQ_HZ                50      // 50 Hz (period = 20000 us)
#define SERVO_MIN_PULSE_US           350     // min pulse width, 0° (calibrated)
#define SERVO_MAX_PULSE_US           1900    // max pulse width, 180° (calibrated), max 2100
#define SERVO_MAX_ANGLE              180 

#define LEDC_TIMER_NUM   LEDC_TIMER_0
#define LEDC_CHANNEL_NUM LEDC_CHANNEL_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES    LEDC_TIMER_14_BIT   // 14 bit (2^14 = 16384)

static const char *TAG = "SERVO_CONTROL";
static int s_servo_angle = 0;

void servo_timer_setup(void) {
    const ledc_timer_config_t timer_cfg = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = SERVO_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    const ledc_channel_config_t channel_cfg = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_NUM,
        .timer_sel      = LEDC_TIMER_NUM,
        .gpio_num       = SERVO_GPIO,
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
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_NUM, pulse));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_NUM));
    ESP_LOGI(TAG, "Set servo angle %d", s_servo_angle);
}

int servo_get_angle() {
    return s_servo_angle;
}

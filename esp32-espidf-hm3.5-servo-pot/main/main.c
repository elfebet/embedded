#include <stdint.h>
#include <stdlib.h>
#include <sys/_intsup.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_log.h"

// Congifure for SG90 (Micro Servo)
#define SERVO_GPIO                   5
#define SERVO_FREQ_HZ                50      // 50 Hz (period = 20000 us)
#define SERVO_TIMEBASE_PERIOD_US     20000   // (1000000 us / 50 Hz)
#define SERVO_MIN_PULSE_US           500     // min pulse width, 0°
#define SERVO_MAX_PULSE_US           2500    // max pulse width, 180°
#define SERVO_MAX_ANGLE              180

#define LEDC_TIMER_NUM   LEDC_TIMER_0
#define LEDC_CHANNEL_NUM LEDC_CHANNEL_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES    LEDC_TIMER_14_BIT   // 14 bit (2^14 = 16384)

#define POT_ADC_UNIT    ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_5 //  esp32-s3 ADC1_5 -> GPIO 6

#define POT_MAX_ANGLE   270
#define POT_START_ANGLE ((POT_MAX_ANGLE - SERVO_MAX_ANGLE) / 2.0f) // 45.0°
#define POT_END_ANGLE   (POT_START_ANGLE + SERVO_MAX_ANGLE)        // 225.0°

static const char *TAG = "SG90_SERVO_POT";
static adc_oneshot_unit_handle_t pot_adc_handle = NULL;

static void servo_timer_init() {
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

static void pot_adc_init() {
    const adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = POT_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &pot_adc_handle));

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12, // 12bits -> 0..4095
        .atten = ADC_ATTEN_DB_12,    // allow measure to 3.3V (the replace default 1.1V)   
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(pot_adc_handle, POT_ADC_CHANNEL, &chan_cfg));
}

static uint32_t angle_to_pulse(uint8_t angle) {
    if (angle >= SERVO_MAX_ANGLE) return SERVO_MAX_PULSE_US;
    return SERVO_MIN_PULSE_US + (angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / SERVO_MAX_ANGLE;
}

static void set_servo_angle(int angle) {
    uint32_t pulse = angle_to_pulse(angle);
    uint32_t max_duty = (1 << LEDC_DUTY_RES) - 1;
    uint32_t duty = (pulse * max_duty) / SERVO_TIMEBASE_PERIOD_US;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_NUM, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_NUM));
}

static uint8_t get_pot_angle(int adc_raw) {
    float angle = ((float)adc_raw / 4095.0f) * POT_MAX_ANGLE;
    if (angle < POT_START_ANGLE) {
        return 0;
    } else if (angle > POT_END_ANGLE) {
        return SERVO_MAX_ANGLE;
    } else {
        return angle - POT_START_ANGLE;
    }
}

static int read_pot_adc_filtered() {
    static int const adc_samples = 12;
    int sum = 0;
    int raw_val = 0;
    for (int i = 0; i < adc_samples; i++) {
        adc_oneshot_read(pot_adc_handle, POT_ADC_CHANNEL, &raw_val);
        sum += raw_val;
    }
    return sum / adc_samples;
}

void app_main(void) {
    servo_timer_init();
    pot_adc_init();

    int adc_raw = 0;
    uint8_t angle = 0, current_angle = 0;
    uint32_t now = 0, current_time = 0;

    while (1) {
        adc_raw = read_pot_adc_filtered();

        angle = get_pot_angle(adc_raw);
        if (abs(current_angle - angle) > 2) {
            current_angle = angle;
            set_servo_angle(angle);
        }
        
        now = esp_timer_get_time();
        if (now - current_time > 500 * 1000) {
            current_time = now;
            ESP_LOGI(TAG, "Angle: %d, raw: %d", angle, adc_raw);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_log.h"

#define LEDC_TIMER_NUM      LEDC_TIMER_0
#define LEDC_MOTOR_CHANNEL  LEDC_CHANNEL_0
#define LEDC_LED_CHANNEL    LEDC_CHANNEL_1
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_RESOLUTION     LEDC_TIMER_10_BIT   // 10bit (2^10 = 1024 values)
#define LEDC_FREQUENCY      20000    // 20 kHz for motor

#define POT_ADC_UNIT    ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_5 //  esp32-s3 ADC1_5 -> GPIO 6
#define MOTOR_PIN       GPIO_NUM_5
#define LED_PIN         GPIO_NUM_7

static const char *TAG = "MOTOR_SPEED";
static adc_oneshot_unit_handle_t pot_adc_handle = NULL;
static uint32_t last_time = 0;

static void timer_and_channels_init() {
    // configure LEDC timer
    const ledc_timer_config_t timer_cfg = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_RESOLUTION,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // configure LEDC channel for MOTOR (connect with timer and GPIO)
    const ledc_channel_config_t motor_channel_cfg = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_MOTOR_CHANNEL,
        .timer_sel      = LEDC_TIMER_NUM,
        .gpio_num       = MOTOR_PIN,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&motor_channel_cfg));

    // configure LEDC channel for LED (connect with timer and GPIO)
    const ledc_channel_config_t led_channel_cfg = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_LED_CHANNEL,
        .timer_sel      = LEDC_TIMER_NUM,
        .gpio_num       = LED_PIN,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel_cfg));
}

static void motor_duty_set(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_MOTOR_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_MOTOR_CHANNEL));
}

static void led_duty_set(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_LED_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_LED_CHANNEL));
}

static void pot_adc_init() {
    const adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = POT_ADC_UNIT };
    adc_oneshot_new_unit(&unit_cfg, &pot_adc_handle);

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12bits -> 0..4095
        .atten = ADC_ATTEN_DB_12,         // allow measure to 3.3V (the replace default 1.1V)   
    };
    adc_oneshot_config_channel(pot_adc_handle, POT_ADC_CHANNEL, &chan_cfg);
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void app_main(void)
{
    printf("Init adc oneshot for POT\n");
    pot_adc_init();
    printf("Init timer + channels for LED and MOTOR\n");
    timer_and_channels_init();

    while (true) {
        int raw = 0;
        adc_oneshot_read(pot_adc_handle, POT_ADC_CHANNEL, &raw);

        uint32_t dutyCycle = map(raw, 0, 4095, 0, 1023);
        led_duty_set(dutyCycle);
        motor_duty_set(dutyCycle);

        uint32_t now = esp_timer_get_time();
        if (now - last_time > 1000 * 1000) {
            last_time = now;
            ESP_LOGI(TAG, "POT raw: %d, duty: %d", raw, dutyCycle);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

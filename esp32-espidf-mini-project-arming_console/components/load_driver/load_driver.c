#include "include/load_driver.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "load_driver";

#define BUZZER_LEDC_TIMER    LEDC_TIMER_0
#define BUZZER_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BUZZER_LEDC_RES      LEDC_TIMER_8_BIT     // duty steps 0..255 -> volume resolution
#define BUZZER_LEDC_FREQ     2000

static uint8_t s_buzzer_volume = 65; // 0..100, for change value use load_buzzer_set_volume()
static bool s_buzzer_on = false;

void load_driver_init() {
    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << LOAD_BUZZER_PIN) | (1ULL << LOAD_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    ledc_timer_config_t timer_cfg = {
        .speed_mode      = BUZZER_LEDC_MODE,
        .duty_resolution = BUZZER_LEDC_RES,
        .timer_num       = BUZZER_LEDC_TIMER,
        .freq_hz         = BUZZER_LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t channel_cfg = {
        .gpio_num   = LOAD_BUZZER_PIN,
        .speed_mode = BUZZER_LEDC_MODE,
        .channel    = BUZZER_LEDC_CHANNEL,
        .timer_sel  = BUZZER_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&channel_cfg);
}

void load_buzzer_update() {
    uint32_t max_duty = (1u << BUZZER_LEDC_RES) - 1u;
    uint32_t duty = (s_buzzer_on && s_buzzer_volume > 0) ? (max_duty * s_buzzer_volume) / 100u : 0u;

    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, duty);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);

    gpio_set_level(LOAD_LED_PIN, s_buzzer_on ? 1 : 0);

//    ESP_LOGI(TAG, "BUZZER PWM duty=%lu/%lu (volume=%u%%, %s)",
//                (unsigned long)duty, (unsigned long)max_duty,
//                 s_buzzer_volume, s_buzzer_on ? "ON" : "OFF");
}

void load_buzzer_set(bool on) {
    s_buzzer_on = on;
    load_buzzer_update();
}

void load_buzzer_chirp(uint32_t ms) {
    load_buzzer_set(true);
    vTaskDelay(pdMS_TO_TICKS(ms));
    load_buzzer_set(false);
}

void load_buzzer_set_volume(uint8_t percent) {
    if (percent > 100) percent = 100;
    s_buzzer_volume = percent;
    load_buzzer_update();
}

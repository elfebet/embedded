#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_timer.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_log.h"

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_LED_CHANNEL    LEDC_CHANNEL_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_RESOLUTION     LEDC_TIMER_12_BIT   // 12bit (2^12 = 4095 values)
//#define LEDC_FREQUENCY      20000 // 20 kHz
#define LEDC_FREQUENCY      5000 // 5 kHz

#define LED_PIN_BLUE    GPIO_NUM_4
#define BTN_PIN         GPIO_NUM_6
#define LED_PIN_RED     GPIO_NUM_7
#define POT_ADC_UNIT    ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_4 //  esp32-s3 ADC1_4 -> GPIO 5

static const char *TAG = "PWM-LED";
static adc_oneshot_unit_handle_t pot_adc_handle = NULL;
static uint32_t last_time = 0;

static void ledc_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_RESOLUTION,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_LED_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .gpio_num       = LED_PIN_RED,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
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

static void button_init() {
    const gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE // rising
    };
    gpio_config(&btn_conf);
}

static void led_blue_init() {
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN_BLUE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN_BLUE, 0);
}

void app_main(void)
{
    ledc_init();
    pot_adc_init();
    button_init();
    led_blue_init();


    int adc_raw = 0;
    ESP_LOGI(TAG, "Potentiometer LED dimmer");

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(pot_adc_handle, POT_ADC_CHANNEL, &adc_raw));
        uint32_t dutyCycle = adc_raw;//map(adc_raw, 0, 4095, 0, 1023);
        led_duty_set(dutyCycle);
        
        bool btn_level = gpio_get_level(BTN_PIN);
        gpio_set_level(LED_PIN_BLUE, btn_level);

        uint32_t now = esp_timer_get_time();
        if (now - last_time > 1000 * 1000) {
            last_time = now;
            ESP_LOGI(TAG, "POT raw: %d, duty: %d, btn level: %d", adc_raw, dutyCycle, btn_level);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

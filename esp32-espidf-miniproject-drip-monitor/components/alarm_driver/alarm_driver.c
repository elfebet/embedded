#include "alarm_driver.h"
#include "config.h"
#include "driver/ledc.h"
#include "driver/sdm.h"

#define BUZZER_TIMER    LEDC_TIMER_0
#define BUZZER_MODE     LEDC_LOW_SPEED_MODE
#define BUZZER_CHANNEL  LEDC_CHANNEL_0
#define BUZZER_RES      LEDC_TIMER_10_BIT
#define SDM_SAMPLE_HZ   500000

static sdm_channel_handle_t s_sdm_chan = NULL;

void alarm_driver_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BUZZER_MODE,
        .duty_resolution = BUZZER_RES,
        .timer_num = BUZZER_TIMER,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = BUZZER_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));

    // led sdm init
    sdm_config_t cfg = {
        .clk_src = SDM_CLK_SRC_DEFAULT,
        .gpio_num = FLOW_LED_PIN,
        .sample_rate_hz = SDM_SAMPLE_HZ,
    };
    ESP_ERROR_CHECK(sdm_new_channel(&cfg, &s_sdm_chan));
    ESP_ERROR_CHECK(sdm_channel_enable(s_sdm_chan));
}

void alarm_buzzer_set(uint32_t freq_hz, uint8_t volume_percent) {
    if (volume_percent > 100) volume_percent = 100;

    ESP_ERROR_CHECK(ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq_hz));
    uint32_t max_duty = (1u << BUZZER_RES) - 1u;
//    uint32_t duty = (max_duty * volume_percent) / 100u / 2u;
    uint32_t duty = (max_duty * volume_percent) / 100u / 2u;
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
}

void alarm_led_set_level(uint8_t percent) {
    if (percent > 100) percent = 100;
    int8_t density = (int8_t)((percent * 255 / 100) - 128);
    ESP_ERROR_CHECK(sdm_channel_set_pulse_density(s_sdm_chan, density));
}

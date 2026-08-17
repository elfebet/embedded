#include "rate_dial.h"
#include "config.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"

#define PCNT_LOW_LIMIT     -32768
#define PCNT_HIGH_LIMIT     32767
#define TICKS_PER_DETENT    4

#define DPM_MIN     5.0f
#define DPM_MAX     80.0f
#define DPM_STEP    1.0f // крапель/хв за один клік диска

static pcnt_unit_handle_t s_pcnt_unit;

void rate_dial_init(void) {
    pcnt_unit_config_t unit_cfg = {
        .low_limit = PCNT_LOW_LIMIT,
        .high_limit = PCNT_HIGH_LIMIT
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &s_pcnt_unit));


    pcnt_glitch_filter_config_t filter_cfg = { .max_glitch_ns = 1000 };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_cfg));

    pcnt_chan_config_t chan_a_cfg = {
        .edge_gpio_num = DIAL_CLK_PIN,
        .level_gpio_num = DIAL_DT_PIN
    };
    pcnt_channel_handle_t chan_a;
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt_unit, &chan_a_cfg, &chan_a));

    pcnt_chan_config_t chan_b_cfg = {
        .edge_gpio_num = DIAL_DT_PIN,
        .level_gpio_num = DIAL_CLK_PIN
    };
    pcnt_channel_handle_t chan_b;
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt_unit, &chan_b_cfg, &chan_b));

    // Протилежна полярність каналів A/B — справжнє X4-декодування
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_a,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_a,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_b,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_b,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(s_pcnt_unit, PCNT_HIGH_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(s_pcnt_unit, PCNT_LOW_LIMIT));

    ESP_ERROR_CHECK(pcnt_unit_enable(s_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(s_pcnt_unit));

    gpio_config_t sw_cfg = {
        .pin_bit_mask = (1ULL << DIAL_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&sw_cfg));
}

float rate_dial_get_target_dpm(void) {
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(s_pcnt_unit, &count));
    float clicks = (float)count / TICKS_PER_DETENT;
    float dpm = 20.0f + clicks * DPM_STEP;   // стартове значення за замовчуванням: 20 крапель/хв
    if (dpm < DPM_MIN) dpm = DPM_MIN;
    if (dpm > DPM_MAX) dpm = DPM_MAX;
    return dpm;
}

bool rate_dial_button_pressed(void) {
    return gpio_get_level(DIAL_SW_PIN) == 0;
}

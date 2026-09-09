#pragma once
#include "rtc_ds1307.h"
#include "bme280.h"

#ifdef __cplusplus
extern "C" {
#endif

void oled_display_init(void);
void oled_play_wolf_boot_animation(uint32_t anim_duration_ms);
void oled_draw_dashboard(
    const rtc_time_t *time,
    const bme280_data_t *bme,
    float threshold_c,
    bool led_on,
    const char *led_source
);

#ifdef __cplusplus
}
#endif
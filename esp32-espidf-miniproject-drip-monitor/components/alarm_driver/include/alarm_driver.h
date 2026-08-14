#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void alarm_driver_init(void);
void alarm_buzzer_set(uint32_t freq_hz, uint8_t volume_percent);
void alarm_led_set_level(uint8_t percent);

#ifdef __cplusplus
}
#endif
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void clamp_servo_init(void); // start ALWAYS in "closed"" state
void clamp_servo_set_open_percent(uint8_t percent); // 0 - fully closed, 100 - fully opened

#ifdef __cplusplus
}
#endif
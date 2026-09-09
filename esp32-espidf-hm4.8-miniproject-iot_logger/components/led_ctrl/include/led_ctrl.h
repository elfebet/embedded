#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void led_ctrl_init(uint32_t led_pin);
void led_ctrl_set(bool on);

#ifdef __cplusplus
}
#endif
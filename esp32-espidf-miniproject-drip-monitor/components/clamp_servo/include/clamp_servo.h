#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void clamp_servo_init(void); // старт ЗАВЖДИ в положенні "закрито"
void clamp_servo_set_open_percent(uint8_t percent); // 0 = повністю закрито, 100 = повністю відкрито

#ifdef __cplusplus
}
#endif
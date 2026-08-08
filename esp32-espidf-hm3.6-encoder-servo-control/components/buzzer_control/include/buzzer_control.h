#pragma once
#include <stdint.h>

#define BUZZER_GPIO 6

void buzzer_setup(void);
void buzzer_play(uint32_t duration_ms);
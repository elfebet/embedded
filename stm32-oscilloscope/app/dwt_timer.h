#pragma once
#include <stdint.h>

void dwt_reset_timer();
uint32_t dwt_get_time();
uint32_t dwt_elapsed_us(uint32_t start_time);
uint32_t dwt_elapsed_ms(uint32_t start_time);

uint32_t dwt_cycles_to_us(uint32_t cycles);
uint32_t dwt_cycles_to_ms(uint32_t cycles);

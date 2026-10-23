#pragma once
#include <stdint.h>

void logger(const char *format, ...);

void logger_reset_timer();
uint32_t logger_get_time();
uint32_t logger_elapsed_us(uint32_t start_time);
uint32_t logger_elapsed_ms(uint32_t start_time);

uint32_t cycles_to_us(uint32_t cycles);
uint32_t cycles_to_ms(uint32_t cycles);

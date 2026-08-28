#pragma once
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    uint8_t sec, min, hour, wday, date, month, year;
} rtc_time_t;

extern const char *const RTC_WEEKDAY[8]; // [0]="" (DS1307 рахує дні з 1), [1]="Sun" ... [7]="Sat"

void rtc_set_address(uint16_t devAddress);
HAL_StatusTypeDef rtc_read_time(rtc_time_t *t);
HAL_StatusTypeDef rtc_set_time(const rtc_time_t *t);
HAL_StatusTypeDef rtc_set_time_from_compile(void);

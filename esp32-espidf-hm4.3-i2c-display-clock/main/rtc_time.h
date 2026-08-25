#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"
#include <unistd.h>

// for DS1307

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t wday;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

static const char *WEEKDAY[8] = {"", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rtc_set_device(i2c_master_bus_handle_t bus_handle, uint16_t device_address);
esp_err_t rtc_read_time(rtc_time_t *t);
esp_err_t rtc_set_time(const rtc_time_t *t);
void rtc_set_time_from_compile(void);
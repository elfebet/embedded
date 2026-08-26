#include "ds1307.h"
#include "i2c.h"   // extern I2C_HandleTypeDef hi2c1;
#include <stdio.h>
#include <string.h>

const char *const RTC_WEEKDAY[8] = {"", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static uint8_t bcd_to_dec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t dec_to_bcd(uint8_t dec) { return (uint8_t)(((dec / 10) << 4) | (dec % 10)); }

uint16_t rtc_address = 0;

void rtc_set_address(uint16_t devAddress) {
    rtc_address = devAddress;
}

HAL_StatusTypeDef rtc_read_time(rtc_time_t *t)
{
    uint8_t raw[7];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, rtc_address, 0x00, I2C_MEMADD_SIZE_8BIT, raw, 7, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;

    t->sec   = bcd_to_dec(raw[0] & 0x7F);   // біт 7 = CH (Clock Halt), не частина значення
    t->min   = bcd_to_dec(raw[1]);
    t->hour  = bcd_to_dec(raw[2] & 0x3F);   // біти 6-7 = режим 12/24г
    t->wday  = raw[3];                      // НЕ BCD -- просте число 1-7
    t->date  = bcd_to_dec(raw[4]);
    t->month = bcd_to_dec(raw[5]);
    t->year  = bcd_to_dec(raw[6]);
    return HAL_OK;
}

HAL_StatusTypeDef rtc_set_time(const rtc_time_t *t)
{
    uint8_t buf[7] = {
        dec_to_bcd(t->sec),
        dec_to_bcd(t->min),
        dec_to_bcd(t->hour),
        t->wday,
        dec_to_bcd(t->date),
        dec_to_bcd(t->month),
        dec_to_bcd(t->year),
    };
    return HAL_I2C_Mem_Write(&hi2c1, rtc_address, 0x00, I2C_MEMADD_SIZE_8BIT, buf, 7, HAL_MAX_DELAY);
}

// Алгоритм Сакамото -- обчислення дня тижня будь-якої григоріанської дати. 0=неділя.
static int day_of_week(int year, int month, int day)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

HAL_StatusTypeDef rtc_set_time_from_compile(void)
{
    static const char *MONTHS = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char month_str[4] = {0};
    int day, year, hour, min, sec;

    sscanf(__DATE__, "%3s %d %d", month_str, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    const char *pos = strstr(MONTHS, month_str);
    int month = pos ? (int)((pos - MONTHS) / 3) + 1 : 1;
    int wday0 = day_of_week(year, month, day);   // 0=Sunday

    rtc_time_t t = {
        .sec = (uint8_t)sec,
        .min = (uint8_t)min,
        .hour = (uint8_t)hour,
        .wday = (uint8_t)(wday0 + 1),// збігається з RTC_WEEKDAY[1]="Sun".. [7]="Sat"
        .date = (uint8_t)day,
        .month = (uint8_t)month,
        .year = (uint8_t)(year % 100),
    };
    return rtc_set_time(&t);
}

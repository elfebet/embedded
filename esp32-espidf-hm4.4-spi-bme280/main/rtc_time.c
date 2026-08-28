#include "rtc_time.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "RTC_time";
static i2c_master_dev_handle_t s_rtc_handle;

uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// Tomohiko Sakamoto's algorithm
int day_of_week(int year, int month, int day) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

time_t get_compile_time_t(void) {
    static const char *MONTHS = "JanFebMarAprMayJunJulAugSepOctNovDec";

    char month_name[4];
    int day, year, hour, min, sec;

    sscanf(__DATE__, "%3s %d %d", month_name, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    const char *pos = strstr(MONTHS, month_name);
    int month = pos ? (int)((pos - MONTHS) / 3) : 0;

    struct tm t = {0};
    t.tm_mday = day;
    t.tm_mon = month;
    t.tm_year = year - 1900; // years after 1900
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    t.tm_isdst = -1;

    time_t compile_timestamp = mktime(&t);
    return compile_timestamp;
}

void rtc_set_device(i2c_master_bus_handle_t bus_handle, uint16_t device_address) {
    i2c_device_config_t rtc_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = 100000 // 100 kHz
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &rtc_cfg, &s_rtc_handle));
}

esp_err_t rtc_read_time(rtc_time_t *t) {
    uint8_t reg = 0x00, raw[7];
    esp_err_t err = i2c_master_transmit_receive(
        s_rtc_handle, 
        &reg, 
        1, 
        raw, 
        7, 
        100
    );
    if (err != ESP_OK) return err;

    t->sec  = bcd_to_dec(raw[0] & 0x7F);
    t->min  = bcd_to_dec(raw[1]);
    t->hour = bcd_to_dec(raw[2] & 0x3F);
    t->wday = raw[3]; // not BCD -- plain number 1-7
    t->date = bcd_to_dec(raw[4]);
    t->month = bcd_to_dec(raw[5]);
    t->year = bcd_to_dec(raw[6]);

    return ESP_OK;
}

esp_err_t rtc_set_time(const rtc_time_t *t) {
    uint8_t buf[8] = {
        0x00,
        dec_to_bcd(t->sec),
        dec_to_bcd(t->min),
        dec_to_bcd(t->hour),
        t->wday,
        dec_to_bcd(t->date),
        dec_to_bcd(t->month),
        dec_to_bcd(t->year),
    };
    return i2c_master_transmit(s_rtc_handle, buf, sizeof(buf), 100);
}

void rtc_set_time_from_compile(void) {
    time_t now = get_compile_time_t() + 30; // 30 sec
    struct tm *time = gmtime(&now);

    int month = time->tm_mon + 1;
    int year = time->tm_year + 1900;
    int wday0 = day_of_week(year, month, time->tm_mday);   // 0=Sunday

    rtc_time_t t = {
        .sec = (uint8_t)time->tm_sec,
        .min = (uint8_t)time->tm_min,
        .hour = (uint8_t)time->tm_hour,
        .wday = (uint8_t)(wday0 + 1), // matches WEEKDAY[1]="Sun" .. WEEKDAY[7]="Sat"
        .date = (uint8_t)time->tm_mday,
        .month = (uint8_t)month,
        .year = (uint8_t)(year % 100),
    };

    esp_err_t err = rtc_set_time(&t);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "DS1307 set to build time: %s %02u.%02u.20%02u  %02u:%02u:%02u",
                 WEEKDAY[t.wday], t.date, t.month, t.year, t.hour, t.min, t.sec);
    } else {
        ESP_LOGE(TAG, "Failed to set DS1307: %s", esp_err_to_name(err));
    }
}

#include "app_main.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "u8g2.h"
#include "u8g2_hal.h"
#include "epd_bitmap_logo.h"
#include "ds1307.h"
#include "bme280.h"

#define SSD1306_ADDR (0x3C << 1)
#define DS1307_ADDR  (0x68 << 1)
#define BME280_REG_ID 0xD0     // адреса регістра id (0x50) з уже встановленим read-бітом

u8g2_t u8g2;

void app_setup() {
    bme280_read_calibration();

    printf("Init SSD1306 \r\n");
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8x8_gpio_and_delay);
    u8x8_SetI2CAddress(&u8g2.u8x8, SSD1306_ADDR);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    rtc_set_address(DS1307_ADDR);
    if (rtc_set_time_from_compile() != HAL_OK) {
        printf("Failed to set DS1307 to build time\r\n");
    } else {
        printf("Set time from compile \r\n");
    }
}

void app_start() {
    printf("Draw logo \r\n");
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawXBM(
            &u8g2,
            (128 - epd_bitmap_width)/2,
            (64 - epd_bitmap_height)/2,
            epd_bitmap_width,
            epd_bitmap_height,
            epd_bitmap_logo
    );
    u8g2_SendBuffer(&u8g2);
    vTaskDelay(pdMS_TO_TICKS(5000));
}

void app_loop() {
    rtc_time_t t = {0};
    if (rtc_read_time(&t) != HAL_OK) {
        printf("Failed to read DS1307\r\n");
    }

    bme280_force_measurement();
    double temperature, pressure, humidity;
    bme280_read_measurements(&temperature, &pressure, &humidity);

    char line_date[16];
    snprintf(line_date, sizeof(line_date), "%s %02u.%02u.20%02u", RTC_WEEKDAY[t.wday], t.date, t.month, t.year);

    char line_time[16];
    snprintf(line_time, sizeof(line_time), "%02u:%02u:%02u", t.hour, t.min, t.sec);

    char line_temp[6];
    snprintf(line_temp, sizeof(line_temp), "%.1fC", temperature);

    char line_meteodata[20];
    snprintf(line_meteodata, sizeof(line_meteodata), "RH=%.0f%% P=%.0fhPa", humidity, pressure);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 8, 14, line_date);
    u8g2_DrawStr(&u8g2, 8, 62, line_meteodata);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 88, 14, line_temp);
    u8g2_SetFont(&u8g2, u8g2_font_logisoso22_tr);
    u8g2_DrawStr(&u8g2, 8, 46, line_time);
    u8g2_SendBuffer(&u8g2);

    printf("Time: %s, Date: %s. %s, %s \r\n", line_time, line_date, line_meteodata, line_temp);
    HAL_Delay(1000);
}

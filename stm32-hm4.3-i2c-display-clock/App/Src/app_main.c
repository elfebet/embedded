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

#define SSD1306_ADDR (0x3C << 1)
#define DS1307_ADDR  (0x68 << 1)

u8g2_t u8g2;

void app_setup() {
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
    vTaskDelay(pdMS_TO_TICKS(10000));
}

void app_loop() {
    rtc_time_t t;
    if (rtc_read_time(&t) == HAL_OK) {
        char line_time[16], line_date[24];
        snprintf(line_time, sizeof(line_time), "%02u:%02u:%02u", t.hour, t.min, t.sec);
        snprintf(line_date, sizeof(line_date), "%s %02u.%02u.20%02u",
                 RTC_WEEKDAY[t.wday], t.date, t.month, t.year);

        printf("[DS1307] time: %s, date: %s \r\n", line_time, line_date);

        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);
        u8g2_DrawStr(&u8g2, 4, 30, line_time);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(&u8g2, 4, 55, line_date);
        u8g2_SendBuffer(&u8g2);
    } else {
        printf("Failed to read DS1307\r\n");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}

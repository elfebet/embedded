#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rtc_time.h" // for DS1307 (serial real-time clock (RTC) chip)
#include "temp_sensor.h" // for DS18B20 (digital temperature sensor)
#include "bme280.h" // for BME280
#include "u8g2.h"
#include "u8g2_hal.h"

#define I2C_PORT       I2C_NUM_0
#define I2C_SDA_PIN    8
#define I2C_SCL_PIN    9

#define SSD1306_ADDR   0x3C // 7bit, value from i2c_scan
#define DS1307_ADDR    0x68
#define DS18B20_PIN    4

#define BME280_SCL_PIN 5    // scl = sck
#define BME280_SDA_PIN 6    // sda = mosi
#define BME280_SDO_PIN 15   // sdo = miso
#define BME280_CSB_PIN 7    // csb = cs

static const char *TAG = "I2C_CLOCK_DISPLAY";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t oled_handle;
static u8g2_t u8g2;

static void i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_PORT,
        .scl_io_num                   = I2C_SCL_PIN,
        .sda_io_num                   = I2C_SDA_PIN,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t oled_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = SSD1306_ADDR,
        .scl_speed_hz    = 400000 // 400 kHz
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &oled_cfg, &oled_handle));
}

static void u8g2_init_display(void) {
    u8g2_hal_set_i2c_device(oled_handle);
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}

static void draw_clock_screen(const rtc_time_t *t, const bme280_data_t *meteo, float temperature) {
    char line_time[16], line_date[24];
    snprintf(line_time, sizeof(line_time), "%02u:%02u:%02u", t->hour, t->min, t->sec);
    snprintf(line_date, sizeof(line_date), "%s %02u.%02u.20%02u", WEEKDAY[t->wday], t->date, t->month, t->year);

    char line_ds_temp[14];
    snprintf(line_ds_temp, sizeof(line_ds_temp), "DS: %.1fC", temperature);

    char line_bme_temp[14];
    snprintf(line_bme_temp, sizeof(line_bme_temp), "BME: %.1fC", meteo->temperature);

    char line_meteo[20];
    snprintf(line_meteo, sizeof(line_meteo), "RH=%.0f%%  P=%.0fhPa", meteo->humidity, meteo->pressure);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 4, 10, line_date);

    u8g2_SetFont(&u8g2, u8g2_font_logisoso20_tr);
    u8g2_DrawStr(&u8g2, 4, 36, line_time);

    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 4, 50, line_ds_temp);
    u8g2_DrawStr(&u8g2, 62, 50, line_bme_temp);
    u8g2_DrawStr(&u8g2, 4, 62, line_meteo);

    u8g2_SendBuffer(&u8g2);
}

void app_main(void) {
    ESP_LOGI("SYSTEM_INFO", "Compiled with IDF version: %s", IDF_VER);

    i2c_bus_init();
    u8g2_init_display();

    temp_sensor_task_create(DS18B20_PIN);

    bme280_spi_bus_and_device_init(
        BME280_SDA_PIN,
        BME280_SDO_PIN,
        BME280_SCL_PIN,
        BME280_CSB_PIN
    );
    bme280_read_calibration();

    rtc_set_device(bus_handle, DS1307_ADDR);
    rtc_set_time_from_compile();

    rtc_time_t time = {0};
    bme280_data_t bme280_data = {0};
    float temperature = 0;
    
    while (true) {
        temperature = temp_sensor_get_temperature();
        ESP_ERROR_CHECK(rtc_read_time(&time));

        bme280_force_measurement();
        bme280_read_measurements(&bme280_data);

        ESP_LOGI(TAG, "DS1307: %s %02u.%02u.20%02u  %02u:%02u:%02u, DS18B20: %.2fC, BME280: T=%.1fC  RH=%.0f%%  P=%.0fhPa",
                WEEKDAY[time.wday],
                time.date, time.month, time.year,
                time.hour, time.min, time.sec,
                temperature,
                bme280_data.temperature, bme280_data.humidity, bme280_data.pressure);

        draw_clock_screen(&time, &bme280_data, temperature);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

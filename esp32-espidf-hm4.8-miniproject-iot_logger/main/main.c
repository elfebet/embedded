#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "bme280.h"
#include "esp_log_timestamp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/adc_types.h"
#include "i2c_bus.h"
#include "env_i2c.h"
#include "env_spi.h"
#include "rtc_ds1307.h"
#include "ctrl_adc.h"
#include "led_ctrl.h"
#include "oled_display.h"
#include "wifi_prov.h"
#include "mqtt_link.h"
#include "cJSON.h"

#define LED_PIN         3
#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_3 // ADC_UNIT_1+ADC_CHANNEL_3 -> GPIO4

#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

#define SPI_SCLK_PIN    5  // scl = sck
#define SPI_MOSI_PIN    6  // sda = mosi
#define SPI_MISO_PIN    15 // sdo = miso
#define SPI_CS_PIN      7  // csb = cs

#define JSON_SEND_DELAY_US (4 * 1000 * 1000) // 4 sec

// Челендж 1: "мертва рука" для дистанційного override -- якщо з моменту
// останньої MQTT-команди минуло більше цього часу, керування LED саме
// повертається до локального порогу (ручки), а не лишається на MQTT
// назавжди (див. обговорення Завдання 9.2).
#define MQTT_OVERRIDE_TIMEOUT_US   (60LL * 1000 * 1000)

static const char *TAG = "IOT_LOGGER";
static volatile bool s_mqtt_override_on = false;
static volatile bool s_mqtt_override_active = false;
static volatile int64_t s_mqtt_override_last_cmd_us = 0;
static int64_t json_send_last_time = 0;

// Викликається з mqtt_link з контексту MQTT-задачі -- лише виставляє прапорці
static void on_mqtt_led_command(bool led_on) {
    s_mqtt_override_on = led_on;
    s_mqtt_override_active = true;
    s_mqtt_override_last_cmd_us = esp_timer_get_time();
}

// Викликається з mqtt_link ОДРАЗУ після успішного SNTP -- "засіює" Tiny RTC
static void on_time_synced(void) {
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    rtc_ds1307_set_from_tm(&timeinfo);
}

static void run_wiring_self_test(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " WIRING SELF-TEST");
    ESP_LOGI(TAG, "========================================");

    rtc_time_t t;
    if (rtc_ds1307_read_time(&t)) {
        ESP_LOGI(TAG, "[OK]   Tiny RTC DS1307 (0x68)  read ok: %02u:%02u:%02u", t.hour, t.min, t.sec);
    } else {
        ESP_LOGE(TAG, "[FAIL] Tiny RTC DS1307 (0x68)  no response -- check VCC/GND/SCL/SDA, battery seated");
    }

    bme280_data_t bme_data = {0};
    bool spi_plausible = env_spi_read(&bme_data);
    if (spi_plausible) {
        ESP_LOGI(TAG, "[OK]   BME280 SPI (CS=%d)  live read: %.1fC %.0f%%RH %.0fhPa",
            SPI_CS_PIN, bme_data.temperature, bme_data.humidity, bme_data.pressure);
    } else {
        ESP_LOGE(TAG, "[FAIL] BME280 SPI (CS=%d)  read=%.1fC -- 0.0/implausible means SPI has NO ack, "
            "a disconnected wire still 'succeeds' with garbage; check SCK/SDI/SDO/CS",
            SPI_CS_PIN, bme_data.temperature);
    }

    ESP_LOGI(TAG, "[--]   LED indicator (GPIO%d)  blinking 3x now -- watch the board", LED_PIN);
    for (int i = 0; i < 3; i++) {
        led_ctrl_set(true);  vTaskDelay(pdMS_TO_TICKS(150));
        led_ctrl_set(false); vTaskDelay(pdMS_TO_TICKS(150));
    }

    ESP_LOGI(TAG, "[--]   Potentiometer (channel_%d)   threshold now = %.1fC -- turn the knob and re-check",
        ADC_CHANNEL, ctrl_adc_get_threshold_c());
    ESP_LOGI(TAG, "========================================");
}

void mqtt_send_json(
    const char *utc_time,
    const bme280_data_t *bme,
    uint32_t adc_raw,
    float threshold_c,
    bool led_on,
    const char *led_source
) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "utc_time", utc_time);
    if (bme != NULL) {
        cJSON_AddBoolToObject(root, "spi_ok", true);
        cJSON_AddNumberToObject(root, "temp_spi_c", bme->temperature);
        cJSON_AddNumberToObject(root, "hum_spi_rh", bme->humidity);
        cJSON_AddNumberToObject(root, "press_spi_hpa", bme->pressure);
    } else {
        cJSON_AddBoolToObject(root, "spi_ok", false);
    }
    cJSON_AddNumberToObject(root, "adc_raw", adc_raw);
    cJSON_AddNumberToObject(root, "threshold_c", threshold_c);
    cJSON_AddBoolToObject(root, "led_on", led_on);
    cJSON_AddStringToObject(root, "led_source", led_source);

    char *json_str = cJSON_PrintUnformatted(root);
    mqtt_link_publish_telemetry(json_str);

    cJSON_free(json_str);
    cJSON_Delete(root);
}

void app_main(void)
{
    ESP_LOGI("SYSTEM_INFO", "Compiled with IDF version: %s", IDF_VER);
    ESP_LOGI("SYSTEM_INFO", "Compiled with С version: %ld", __STDC_VERSION__);

    i2c_bus_init(I2C_SDA_PIN, I2C_SCL_PIN, I2C_PORT); // init only once for any I2C 
    oled_display_init();

    wifi_prov_init();
    if (wifi_prov_is_provisioned()) {
        oled_draw_text("Connect to WiFi...");
        wifi_prov_connect_to_saved_wifi();
    } else {
        oled_draw_text("Start provisioning...");
        wifi_prov_start_provisioning();
    }

//    env_i2c_init();
    rtc_ds1307_init();
    led_ctrl_init(LED_PIN);
    env_spi_init(SPI_MOSI_PIN, SPI_MISO_PIN, SPI_SCLK_PIN, SPI_CS_PIN);
    ctrl_adc_init(ADC_UNIT, ADC_CHANNEL);

    run_wiring_self_test();

//    ESP_LOGI(TAG, "Showing boot wolf animation...");
//    oled_play_wolf_boot_animation(3000);

    mqtt_link_init(on_mqtt_led_command, on_time_synced);  // broker/client ID from idf.py menuconfig

    rtc_time_t time = {0};
    bme280_data_t spi_bme_data = {0};
//    bme280_data_t i2c_bme_data = {0};
    
    while (1) {
        bool rtc_ok = rtc_ds1307_read_time(&time);
//        bool i2c_ok = env_i2c_read(&i2c_bme_data);
        bool spi_ok = env_spi_read(&spi_bme_data);

        float threshold_c = ctrl_adc_get_threshold_c();
        uint32_t adc_raw = ctrl_adc_get_raw();
        uint32_t now = esp_timer_get_time();

        // Челендж 1: "мертва рука" -- override з MQTT сам звільняє керування
        // назад ручці, якщо давно не було нової команди (а не висить вічно,
        // як обговорювалось у Завданні 9.2).
        if (s_mqtt_override_active && (now - s_mqtt_override_last_cmd_us) > MQTT_OVERRIDE_TIMEOUT_US) {
            s_mqtt_override_active = false;
            ESP_LOGI(TAG, "MQTT override timed out (%lld s) -- returning LED control to the local threshold",
                     (long long)(MQTT_OVERRIDE_TIMEOUT_US / 1000000));
        }

        // --- Рішення про LED: локальний поріг АБО дистанційний override з MQTT ---
        bool local_alarm = spi_ok && spi_bme_data.temperature > threshold_c;
        bool led_on = s_mqtt_override_active ? s_mqtt_override_on : local_alarm;
        led_ctrl_set(led_on);

        const char *led_source = s_mqtt_override_active ? "mqtt" : "local";
        char utc_time[16] = "--:--:--";

        if (rtc_ok) {
            oled_draw_dashboard(&time, &spi_bme_data, threshold_c, led_on, led_source);
            snprintf(utc_time, sizeof(utc_time), "%02u:%02u:%02u", time.hour, time.min, time.sec);
        }

        if (now - json_send_last_time > JSON_SEND_DELAY_US) {
            json_send_last_time = now;
            mqtt_send_json(
                utc_time,
                spi_ok ? &spi_bme_data : NULL,
                adc_raw,
                threshold_c,
                led_on,
                led_source
            );
        }

        ESP_LOGI(TAG, "%s | SPI T=%.1fC | threshold=%.1fC | LED=%s(%s) | MQTT=%s",
                 utc_time, spi_bme_data.temperature, threshold_c, led_on ? "ON" : "OFF", led_source,
                 mqtt_link_is_connected() ? "up" : "down");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

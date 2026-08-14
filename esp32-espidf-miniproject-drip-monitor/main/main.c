//#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "drop_sensor.h"
#include "temp_sensor.h"
#include "rate_dial.h"
#include "clamp_servo.h"
#include "alarm_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "drip_monitor";

#define STALL_TIMEOUT_MS 8000 // немає крапель довше — вважаємо потік зупиненим

void app_main(void) {
    drop_sensor_init();
    temp_sensor_init();
    rate_dial_init();
    clamp_servo_init();
    alarm_driver_init();

    int8_t open_percent = 30;   // початкове положення відкриття, скориговане контуром нижче
    bool silenced = false;

    while (1) {
        if (drop_sensor_poll_event()) {
            drop_sensor_register_event();
        }

        float actual_dpm = drop_sensor_get_rate_dpm();
        float target_dpm = rate_dial_get_target_dpm();
        bool stalled = drop_sensor_is_stalled(STALL_TIMEOUT_MS);

        float temp_c, temp_err_c;
        bool temp_ok = temp_sensor_read_celsius(&temp_c, &temp_err_c);

        // --- Керування затискачем: просте пропорційне підстроювання ---
        if (stalled || !temp_ok) {
            open_percent = 0;   // БЕЗУМОВНЕ закриття при відмові будь-якого критичного датчика
        } else {
            float error_dpm = target_dpm - actual_dpm;
            open_percent += (uint8_t)(error_dpm > 0 ? 1 : (error_dpm < 0 ? -1 : 0));
            if (open_percent > 100) open_percent = 100;
            if (open_percent < 0)   open_percent = 0;
        }
        clamp_servo_set_open_percent(open_percent);

        // --- Тривога: ступінчаста за відхиленням від цілі ---
        float deviation_pct = (target_dpm > 0) ? (100.0f * (target_dpm - actual_dpm) / target_dpm) : 0;
        if (deviation_pct < 0) deviation_pct = -deviation_pct;

        if (stalled || !temp_ok) {
            alarm_buzzer_set(2500, silenced ? 0 : 100);   // максимальна тривога
            alarm_led_set_level(100);
        } else if (deviation_pct > 30.0f) {
            alarm_buzzer_set(1500, silenced ? 0 : 60);
            alarm_led_set_level((uint8_t)(actual_dpm * 100 / 80));
        } else {
            alarm_buzzer_set(1000, 0);   // в межах норми — тиша
            alarm_led_set_level((uint8_t)(actual_dpm * 100 / 80));
        }

        // --- Кнопка диска: тимчасово вимкнути звук (не саму тривогу — LED лишається) ---
        static bool sw_prev = false;
        bool sw_now = rate_dial_button_pressed();
        if (sw_now && !sw_prev) {
            silenced = !silenced;
            ESP_LOGW(TAG, "звук тривоги: %s", silenced ? "вимкнено вручну" : "увімкнено");
        }
        sw_prev = sw_now;

        ESP_LOGI(TAG, "темп=%.1f/%.1f крапель/хв | t=%s%.1f°C | відкриття=%u%% | %s",
                 actual_dpm,
                 target_dpm,
                 temp_ok ? "" : "ПОМИЛКА ",
                 temp_ok ? temp_c : 0.0f,
                 open_percent, stalled ? "ЗУПИНКА ПОТОКУ" : "ОК");

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

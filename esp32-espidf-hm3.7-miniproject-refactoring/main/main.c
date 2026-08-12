#include <stdio.h>
//#include <unistd.h>
#include <stdint.h>
#include "ldr.h"
#include "servo.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_timer.h"

#define LDR_MIN_VALUE 1000
#define LDR_MAX_VALUE 3000
#define LOG_DELAY_US  500000 // 500 ms

static const char *TAG = "Mini Project";


static long constrain(long x, long min_val, long max_val) {
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

static long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void app_main(void)
{
    ldr_init();
    servo_init();

    int raw = 0, angle = 0;
    uint32_t now = 0, current_time = 0;

    while(1) {
        raw = ldr_raw_value(); // 0..4095
        angle = map(raw, LDR_MIN_VALUE, LDR_MAX_VALUE, 0, 180);
        angle = constrain(angle, 0, 180);
        servo_set_angle(angle);

        now = esp_timer_get_time();
        if (now - current_time > LOG_DELAY_US) {
            current_time = now;
            ESP_LOGI(TAG, "Servo angle: %d, ldr raw: %d", angle, raw);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
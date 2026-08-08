#include <stdbool.h>
#include <unistd.h>
#include "servo_control.h"
#include "encoder_control.h"
#include "buzzer_control.h"
#include "esp_log.h"

static const char *TAG = "ENCODER_APP";

static int angle_step = 30;
#define MIN_ANGLE 0
#define MAX_ANGLE 180

void on_short_click(void) {
    angle_step = (angle_step == 30) ? 15  :30;
    ESP_LOGI(TAG, "[SHORT PRESS] Switch to step: %d", angle_step);
}

void on_long_click(void) {
    servo_set_angle(90);
    ESP_LOGI(TAG, "[LONG PRESS] Reset to 90 degree");
}

void on_rotate(int direction) {
    int angle = servo_get_angle();
    ESP_LOGI(TAG, "on_rotate direction: %d", direction);

    if (direction == 1) {
        if (angle < MAX_ANGLE && angle+angle_step > MAX_ANGLE) {
            angle = MAX_ANGLE;
        } else {
            angle += angle_step;
        }
    } else if (direction == -1) {
        if (angle > MIN_ANGLE && angle-angle_step < MIN_ANGLE) {
            angle = MIN_ANGLE;
        } else {
            angle -= angle_step;
        }
    } else {
        ESP_LOGE(TAG, "Unknown direction in \"on_rotate\"");
        return;
    }

    if (angle >= MIN_ANGLE && angle <= MAX_ANGLE) {
        servo_set_angle(angle);
    } else {
        buzzer_play(100);
    }
}

void app_main(void) {
    servo_timer_setup();
    encoder_setup();
    encoder_register_rotation(on_rotate);
    encoder_register_short_press(on_short_click);
    encoder_register_long_press(on_long_click);

    buzzer_setup();
    servo_set_angle(90);
}

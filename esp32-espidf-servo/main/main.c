#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Congifure for SG90 (Micro Servo)
#define SERVO_GPIO                   5
#define SERVO_FREQ_HZ                50      // 50 Гц (період = 20000 мкс)
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1 МГц (дискретність 1 мкс)
#define SERVO_TIMEBASE_PERIOD_US     20000   // Повний період (1000000 us / 50 Hz)
#define SERVO_MIN_PULSE_US           500     // Мінімальна ширина імпульсу (0 deg)
#define SERVO_MAX_PULSE_US           2500    // Максимальна ширина імпульсу (180 deg)
#define SERVO_MAX_DEGREE             180     // Максимальний кут повороту

static const char *TAG = "SG90_SERVO_EXAMPLE";

static uint32_t angle_to_pulse(uint8_t angle) {
    if (angle >= SERVO_MAX_DEGREE) return SERVO_MAX_PULSE_US;
    return SERVO_MIN_PULSE_US + (angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / SERVO_MAX_DEGREE;
}

#define MCPWM_SERVO 0

#if MCPWM_SERVO
#include "driver/mcpwm_prelude.h"

mcpwm_cmpr_handle_t comparator = NULL;

static void servo_timer_init() {
    // Init MCPWM timer
    mcpwm_timer_handle_t timer = NULL;
    const mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD_US,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    // Init MCPWM operator
    mcpwm_oper_handle_t oper = NULL;
    const mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator and timer should be in the same group, i.e. 0
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    // create comparator
    const mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    // create  MCPWM generator
    mcpwm_gen_handle_t generator = NULL;
    const mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = SERVO_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // configure generator actions on  timer and compare events
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    // start timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

static void servo_set_angle(uint8_t angle) {
    uint32_t pulse = angle_to_pulse(angle);
    ESP_LOGI(TAG, "Встановлення кута: %d, ширина імпульсу: %d", angle, pulse);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, pulse));
}

#else
#include "driver/ledc.h"

#define LEDC_TIMER_NUM   LEDC_TIMER_0
#define LEDC_CHANNEL_NUM LEDC_CHANNEL_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES    LEDC_TIMER_14_BIT   // Роздільна здатність 14 біт (2^14 = 16384 значень)

static void servo_timer_init() {
    // configure LEDC timer
    const ledc_timer_config_t timer_cfg = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = SERVO_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // configure LEDC channel (connect with timer and GPIO)
    const ledc_channel_config_t channel_cfg = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_NUM,
        .timer_sel      = LEDC_TIMER_NUM,
        .gpio_num       = SERVO_GPIO,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

static void servo_set_angle(int angle) {
    uint32_t pulse = angle_to_pulse(angle);

    uint32_t max_duty = (1 << LEDC_DUTY_RES) - 1;   // 16383 для 14 біт
    uint32_t duty = (pulse * max_duty) / SERVO_TIMEBASE_PERIOD_US;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_NUM, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_NUM));
    ESP_LOGI(TAG, "Встановлення кута: %d, ширина імпульсу: %d, duty: %d", angle, pulse, duty);
}
#endif


void app_main(void) {
    servo_timer_init();

    uint8_t angle = 0;
    const uint8_t step = 15;
    while (1) {
        servo_set_angle(angle);
        vTaskDelay(pdMS_TO_TICKS(500));
        angle += step;
        if (angle > SERVO_MAX_DEGREE) {
            angle = 0;
        }
    }
}
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_types.h"
#include "driver/mcpwm_prelude.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "tt_motor";

#define MOTOR_PWMA_GPIO      7
#define MOTOR_AIN1_GPIO      6
#define MOTOR_AIN2_GPIO      5

#define PWM_FREQ_HZ          20000  // 20 кГц
#define PWM_PERIOD_TICKS     1000

static mcpwm_cmpr_handle_t comparator;

void init_motor(void) {
    ESP_LOGI(TAG, "Init GPIO for move direction...");
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_AIN1_GPIO) | (1ULL << MOTOR_AIN2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Init MCPWM timer...");
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = PWM_FREQ_HZ * PWM_PERIOD_TICKS,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = PWM_PERIOD_TICKS,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    ESP_LOGI(TAG, "Init MCPWM operator...");
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Init MCPWM comparator...");
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    ESP_LOGI(TAG, "Init MCPWM generator...");
    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = MOTOR_PWMA_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // set pwd to 0 (motor is stopped)
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, 0));

    // configure generator behaviour: HIGH level at start, LOW level upon match with comparator
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Start MCPWM timer...");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

// Speed from -100 to 100, duty > 0: forward, duty < 0: backward
void set_motor_speed(int duty_cycle) {
    if (duty_cycle > 100) duty_cycle = 100;
    if (duty_cycle < -100) duty_cycle = -100;

    if (duty_cycle > 0) {
        // forward
        gpio_set_level(MOTOR_AIN1_GPIO, 1);
        gpio_set_level(MOTOR_AIN2_GPIO, 0);
        uint32_t compare_val = (duty_cycle * PWM_PERIOD_TICKS) / 100;
        mcpwm_comparator_set_compare_value(comparator, compare_val);
    } else if (duty_cycle < 0) {
        // backward
        gpio_set_level(MOTOR_AIN1_GPIO, 0);
        gpio_set_level(MOTOR_AIN2_GPIO, 1);
        uint32_t compare_val = (-duty_cycle * PWM_PERIOD_TICKS) / 100;
        mcpwm_comparator_set_compare_value(comparator, compare_val);
    } else {
        // braking (Short brake)
        gpio_set_level(MOTOR_AIN1_GPIO, 1);
        gpio_set_level(MOTOR_AIN2_GPIO, 1);
        mcpwm_comparator_set_compare_value(comparator, 0);
    }
}

void app_main(void) {
    init_motor();

    while (1) {
        ESP_LOGI(TAG, "Forward with 50%% speed");
        set_motor_speed(50);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "Forward with 100%% speed");
        set_motor_speed(100);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "Braking");
        set_motor_speed(0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Backward with 70%% speed");
        set_motor_speed(-70);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "Backward with 70%% speed");
        set_motor_speed(-30);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "Braking");
        set_motor_speed(0);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

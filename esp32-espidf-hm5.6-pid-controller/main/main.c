#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// TB6612FNG driver
#define GPIO_STBY       GPIO_NUM_8
#define GPIO_PWM_A      GPIO_NUM_7
#define GPIO_AIN1       GPIO_NUM_6
#define GPIO_AIN2       GPIO_NUM_5

#define BUTTON_GPIO     GPIO_NUM_0

// LEDC
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES       LEDC_TIMER_10_BIT // 10bit (2^10 = 1024 values)
#define LEDC_FREQUENCY      5000 // 5 kHz
#define LEDC_MAX_DUTY       ((1 << LEDC_DUTY_RES) - 1)

#define PID_DT_S              0.05f // 50 milliseconds
#define PID_PERIOD_US         50000 // 50 milliseconds

typedef struct {
    float kp, ki, kd;
    float dt;
    float integral;
    float integral_min;
    float integral_max;
    float prev_measurement; // the same as prev_error
    float d_filtered;
    float d_alpha;          // derivative filter: d_filtered = alpha*d_prev + (1-alpha)*d_new
    bool  enable_i;
    bool  enable_d;
} pid_data_t;

// Mathematical model of a DC motor with inertia and friction
typedef struct {
    float speed_rpm;    // current modeled speed
    float k_gain;       // gain coefficient (RPM * PWD percent, 100% = 200 RPM)
    float friction;     // friction/damping coefficient
} motor_plant_t;

static void pid_init(pid_data_t *p, float kp, float ki, float kd, float dt, float integral_min, float integral_max) {
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
    p->dt = dt;

    p->integral = 0.0f;
    p->integral_min = integral_min;
    p->integral_max = integral_max;

    p->prev_measurement = 0.0f;
    p->d_filtered = 0.0f;
    p->d_alpha = 0.8f;

    p->enable_i = true;
    p->enable_d = true;
}

static float pid_compute(pid_data_t *p, float setpoint, float measurement) {
    float error = setpoint - measurement;

    // proportional term
    float p_term = p->kp * error;

    // integral term
    float i_term = 0.0f;
    if (p->enable_i) {
        p->integral += error * p->dt;
        if (p->integral > p->integral_max) p->integral = p->integral_max;
        if (p->integral < p->integral_min) p->integral = p->integral_min;
        i_term = p->ki * p->integral;
    }

    // derivative term
    float d_term = 0.0f;
    if (p->enable_d) {
        float d_raw = (measurement - p->prev_measurement) / p->dt;
        p->d_filtered = p->d_alpha * p->d_filtered + (1.0f - p->d_alpha) * d_raw;
        d_term = p->kd * p->d_filtered;
    }

    p->prev_measurement = measurement;
    
    float output = p_term + i_term + d_term;
    // Clamp the PID output to PWM limits (0–100%)
    if (output > 100.0f) output = 100.0f;
    if (output < 0.0f)   output = 0.0f;

    return output;
}

float motor_plant_update(motor_plant_t *plant, float pwm_percent, float dt) {
    // d(Speed)/dt = (K * Duty - Friction * Speed)
    float accel = (plant->k_gain * pwm_percent - plant->friction * plant->speed_rpm);
    plant->speed_rpm += accel * dt;
    if (plant->speed_rpm < 0.0f) plant->speed_rpm = 0.0f;
    return plant->speed_rpm;
}

static void init_tb6612fng(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_AIN1) | (1ULL << GPIO_AIN2) | (1ULL << GPIO_STBY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // activate driver and set forward movement
    ESP_ERROR_CHECK(gpio_set_level(GPIO_STBY, 1));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_AIN1, 1));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_AIN2, 0));

    // configure PWD
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .gpio_num       = GPIO_PWM_A,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void set_physical_motor_pwm(float duty_percent) {
    if (duty_percent < 0.0f) duty_percent = 0.0f;
    if (duty_percent > 100.0f) duty_percent = 100.0f;

    // convert percent to 10bit value (0..1023)
    uint32_t duty_raw = (uint32_t)((duty_percent / 100.0f) * (float)LEDC_MAX_DUTY);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty_raw);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void button_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

// gptimer: hardware timing for PID-loop, 50 ms
static TaskHandle_t pid_task_handle = NULL;

static bool IRAM_ATTR timer_alarm_cb(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx
) {
    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(pid_task_handle, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

static void pid_timer_init(void) {
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz -> 1 tick = 1 us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = PID_PERIOD_US,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

static float setpoint_rpm = 120.0f;

static void check_button_and_advance_setpoint(void) {
    static bool prev_pressed = false;
    static int64_t last_change_us = 0;

    bool pressed = (gpio_get_level(BUTTON_GPIO) == 0);
    int64_t now_us = esp_timer_get_time();

    if (pressed && !prev_pressed && (now_us - last_change_us > 50000)) {
        if (setpoint_rpm == 120.0) {
            setpoint_rpm = 180.0;
        } else if (setpoint_rpm == 180.0) {
            setpoint_rpm = 80.0;
        } else /*if (setpoint_rpm == 80.0)*/ {
            setpoint_rpm = 120.0;
        }
        last_change_us = now_us;
    }
    prev_pressed = pressed;
}

// smooth adjustment of the active "setpoint"" to the target (Ramp)
static float apply_setpoint_ramp(float current_sp, float target_sp, float max_change_per_sec, float dt) {
    float max_step = max_change_per_sec * dt;
    float diff = target_sp - current_sp;

    if (diff > max_step) return current_sp + max_step;
    if (diff < -max_step) return current_sp - max_step;
    return target_sp;
}

void app_main(void) {
    pid_task_handle = xTaskGetCurrentTaskHandle();

    init_tb6612fng();
    button_init();
    pid_timer_init();

    pid_data_t pid;
    pid_init(&pid, 0.8, 0.5, 0.01, PID_DT_S, 0.0f, 100.0f);
//    pid_init(&pid, 15.0, 0.0, 0.0, PID_DT_S, 0.0, 100.0f); // 1
//    pid_init(&pid, 2.0, 0.5, 0.1, PID_DT_S, 0.0f, 100.0f); // 3
//    pid_init(&pid, 2.5, 1.25, 0.05, PID_DT_S, 0.0f, 100.0f); // 4
//    pid_init(&pid, 1.2, 2.5, 0.02, PID_DT_S, 0.0f, 100.0f); // 5
//    pid_init(&pid, 0.8, 1.5, 0.01, PID_DT_S, 0.0f, 100.0f); //6
//    pid_init(&pid, 0.8, 0.5, 0.01, PID_DT_S, 0.0f, 100.0f); // 7

    printf("t_ms,setpoint,current,pwd_out\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    motor_plant_t virtual_motor = {
        .speed_rpm = 0.0f,
        .k_gain = 2.5f,      // Model: 100% PWM = 200 RPM
        .friction = 1.25f
    };

    setpoint_rpm = 120.0;
    float active_setpoint_rpm = 120.0f;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        check_button_and_advance_setpoint();

        // gradually bring active_setpoint_rpm closer to target_setpoint_rpm (max. 40 RPM per second)
        active_setpoint_rpm = apply_setpoint_ramp(active_setpoint_rpm, setpoint_rpm, 40.0f, PID_DT_S);

        // PID calculates the required PWM percentage.
        float control_pwm = pid_compute(&pid, active_setpoint_rpm, virtual_motor.speed_rpm);
        
        // set PWD to TB6612FNG (the motor spins), 0..100%
        set_physical_motor_pwm(control_pwm);
        
        // send PWD to virtual model (receive simulated feedback)
        float current_rpm = motor_plant_update(&virtual_motor, control_pwm, PID_DT_S);

        printf("%lld,%.1f,%.1f,%.1f\n", esp_timer_get_time() / 1000, setpoint_rpm, current_rpm, control_pwm);
   }
}

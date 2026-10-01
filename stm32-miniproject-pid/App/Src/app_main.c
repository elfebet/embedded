#include "app_main.h"
#include "main.h"
#include "drv_encoder.h"
#include "drv_adc.h"
#include "drv_pwm.h"
#include "drv_motor.h"
#include "leds_task.h"
#include "input_task.h"
#include "motor_task.h"
#include "regulator_task.h"
#include "logger_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

static encoder_t encoder;
static encoder_button_t button;
static motor_driver_t motor_drv;
static pwm_t pwm; // regulator output
static adc_t adc; // regulator input

void configure_adc(void) {
    // adc on PA1 pin
    adc.hadc = &hadc1;
    adc.oversample = 8u; // for average adc with 8 raws
    adc.vref_mv = 3300u; // 3.3 V
}

void configure_pwm(void) {
    // pwm on PA6 pin
    pwm.htim = &htim3;
    pwm.channel = TIM_CHANNEL_1;
}

void configure_encoder(void) {
    encoder.htim = &htim4;
    encoder.counts_per_detent = 4; // calculate 4 changes per detent
    encoder.invert = true;
    encoder.remainder = 0;
    encoder.last_cnt = 0;

    // encoder button
    button.port = ENC_SW_GPIO_Port;
    button.pin = ENC_SW_Pin;
    button.threshold = 3u; // debounce 30ms (3 * 10u tick)
    button.count = 0;
    button.stable = false;
}

void configure_motor_drv(void) {
    motor_drv.in1 = (motor_driver_pin_t){
        .port = MOTOR_AIN1_GPIO_Port,
        .pin = MOTOR_AIN1_Pin
    };
    motor_drv.in2 = (motor_driver_pin_t){
        .port = MOTOR_AIN2_GPIO_Port,
        .pin = MOTOR_AIN2_Pin
    };
    motor_drv.stby = (motor_driver_pin_t){
        .port = MOTOR_STBY_GPIO_Port,
        .pin = MOTOR_STBY_Pin
    };
}

void app_init() {
    configure_adc();
    configure_pwm();
    configure_encoder();
    configure_motor_drv();
}

void app_start(void) {
    logger_task_init(osPriorityNormal);
    logger("app start");

    unsigned long tskRegulatorPriority = osPriorityAboveNormal+4;
    unsigned long tskMotorPriority = osPriorityAboveNormal+3;
    unsigned long tskInputPriority = osPriorityAboveNormal+2;

    motor_driver_init(&motor_drv);
    encoder_button_init(&button);
    encoder_start(&encoder);
    pwm_start(&pwm);

    const motor_fsm_config_t config = {
        .deadtime_ms = 150u, // pause when change direction
        .idle_timeout_ms = 600u  // stop delay when the encoder inactive, 0 = non-stop
    };
    motor_task_init(motor_driver_as_motor(&motor_drv), &config, tskMotorPriority);
    leds_task_init();

    regulator_task_init(&pwm, &adc, tskRegulatorPriority);
    input_task_init(&encoder, &button, tskInputPriority);

    logger("app start end");
}

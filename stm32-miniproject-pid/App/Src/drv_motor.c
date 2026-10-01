#include "drv_motor.h"

void motor_driver_stop(void *self) {
    motor_driver_t *m = (motor_driver_t *)self;
    HAL_GPIO_WritePin(m->stby.port, m->stby.pin, GPIO_PIN_RESET);
}

void motor_driver_run(void *self, motor_direction_t dir) {
    motor_driver_t *m = (motor_driver_t *)self;
    HAL_GPIO_WritePin(m->in1.port, m->in1.pin, (dir == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(m->in2.port, m->in2.pin, (dir == MOTOR_DIR_CCW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(m->stby.port, m->stby.pin, GPIO_PIN_SET);
}

static const motor_ops_t MOTOR_DRIVER_OPS = {
    .run  = motor_driver_run,
    .stop = motor_driver_stop,
};

void motor_driver_init(const  motor_driver_t *driver) {
    motor_driver_stop(driver);
}

motor_t motor_driver_as_motor(const motor_driver_t *driver) {
    motor_t motor = { .ops = &MOTOR_DRIVER_OPS, .self = driver };
    return motor;
}

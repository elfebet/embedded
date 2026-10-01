#pragma once

// Motor driver TB6612FNG

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include "motor_fsm.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} motor_driver_pin_t;

typedef struct {
    motor_driver_pin_t in1;
    motor_driver_pin_t in2;
    motor_driver_pin_t stby;
} motor_driver_t;

void motor_driver_init(const motor_driver_t *driver);
motor_t motor_driver_as_motor(const motor_driver_t *driver); // return as abstract motor

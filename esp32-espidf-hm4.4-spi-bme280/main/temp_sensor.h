#pragma once
#include <stdint.h>

// for DS18B20

void temp_sensor_task_create(uint8_t pin);
float temp_sensor_get_temperature();
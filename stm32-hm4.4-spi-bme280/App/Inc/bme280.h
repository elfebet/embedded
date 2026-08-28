#pragma once
#include <stdint.h>
#include "stm32f4xx_hal.h"

HAL_StatusTypeDef bme280_read_regs(uint8_t reg, uint8_t *out, uint16_t len);
HAL_StatusTypeDef bme280_write_reg(uint8_t reg, uint8_t value);

void bme280_read_calibration(void);
void bme280_force_measurement(void);
void bme280_read_measurements(double *temperature, double *pressure, double *humidity);

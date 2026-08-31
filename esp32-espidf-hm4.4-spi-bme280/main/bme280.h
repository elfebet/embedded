#pragma once
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    double temperature, pressure, humidity;
} bme280_data_t;

void bme280_spi_bus_and_device_init(int mosi_pin, int miso_pin, int sclk_pin, int cs_pin);
void bme280_check_device(void);

esp_err_t bme280_read_regs(uint8_t reg, uint8_t *out, size_t len);
esp_err_t bme280_write_reg(uint8_t reg, uint8_t value);

void bme280_read_calibration(void);
void bme280_force_measurement(void);
void bme280_read_measurements(bme280_data_t *data);

#pragma once
#include <stdbool.h>
#include "bme280.h"

#ifdef __cplusplus
extern "C" {
#endif

void env_spi_init(int mosi_pin, int miso_pin, int sclk_pin, int cs_pin);
bool env_spi_read(bme280_data_t *out);

#ifdef __cplusplus
}
#endif
#pragma once
#include <stdbool.h>
#include "bme280.h"

#ifdef __cplusplus
extern "C" {
#endif

void env_i2c_init(void);
bool env_i2c_read(bme280_data_t *out);

#ifdef __cplusplus
}
#endif
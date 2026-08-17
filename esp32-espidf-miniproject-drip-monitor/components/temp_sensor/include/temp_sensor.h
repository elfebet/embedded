#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void temp_sensor_init(void);
uint16_t temp_sensor_read_raw();
bool temp_sensor_read_celsius(float *out_c, float *out_err_c);

#ifdef __cplusplus
}
#endif
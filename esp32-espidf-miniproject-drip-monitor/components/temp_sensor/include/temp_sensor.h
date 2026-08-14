#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void temp_sensor_init(void);
bool temp_sensor_read_celsius(float *out_c, float *out_err_c);

#ifdef __cplusplus
}
#endif
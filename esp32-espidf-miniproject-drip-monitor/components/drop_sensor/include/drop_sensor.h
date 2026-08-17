#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void drop_sensor_init(void);
uint16_t drop_sensor_read_raw(void);
bool drop_sensor_poll_event(void); // true for each drop detected
void drop_sensor_register_event(void);
float drop_sensor_get_rate_dpm(void); // drop/minute
bool drop_sensor_is_stalled(uint32_t timeout_ms); // no drops longer than timeout

#ifdef __cplusplus
}
#endif
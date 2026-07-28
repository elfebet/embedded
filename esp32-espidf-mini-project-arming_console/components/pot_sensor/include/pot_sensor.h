#pragma once 
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pot_sensor_init(void); // start periodic esp_timer to 50 ms
uint16_t pot_sensor_get_raw(void); // last read value ADC 0-4095, 12bits ((ESP32)) -> (cache, not blocking) 

#ifdef __cplusplus
}
#endif

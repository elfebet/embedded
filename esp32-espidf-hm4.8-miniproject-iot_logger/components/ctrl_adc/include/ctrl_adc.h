#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ctrl_adc_init(uint8_t unit, uint8_t channel);
float ctrl_adc_get_threshold_c(void);   // current threshold °C, 15.0...35.0
uint32_t ctrl_adc_get_raw(void);        // adc raw data 0..4095 

#ifdef __cplusplus
}
#endif
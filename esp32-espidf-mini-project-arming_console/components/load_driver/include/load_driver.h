#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pin -> Resistor (220 Ohm) -> Transistor (BC547B) -> Buzzer 
#define LOAD_BUZZER_PIN GPIO_NUM_10
#define LOAD_LED_PIN GPIO_NUM_11 // led for simulate buzzer

void load_driver_init(void);
void load_buzzer_set(bool on);
void load_buzzer_chirp(uint32_t ms);
void load_buzzer_set_volume(uint8_t percent);

#ifdef __cplusplus
}
#endif
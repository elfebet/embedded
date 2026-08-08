#pragma once

#define ENCODER_GPIO_A      9
#define ENCODER_GPIO_B      10
#define ENCODER_GPIO_BUTTON 11

typedef void (*encoder_rotation_callback_t)(int direction);
typedef void (*encoder_button_callback_t)(void);

void encoder_setup(void);

void encoder_register_rotation(encoder_rotation_callback_t callback);
void encoder_register_short_press(encoder_button_callback_t callback);
void encoder_register_long_press(encoder_button_callback_t callback);
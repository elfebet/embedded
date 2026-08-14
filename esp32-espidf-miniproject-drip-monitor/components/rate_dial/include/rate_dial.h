#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void rate_dial_init(void);
float rate_dial_get_target_dpm(void); // 5..80 крапель/хв
bool rate_dial_button_pressed(void); // "озвучити тривогу / тиша"

#ifdef __cplusplus
}
#endif
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void rate_dial_init(void);
float rate_dial_get_target_dpm(void); // 5..80 drops per minute
bool rate_dial_button_pressed(void); // "play alarm / silence"

#ifdef __cplusplus
}
#endif
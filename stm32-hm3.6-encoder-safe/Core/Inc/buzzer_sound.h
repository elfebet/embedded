/*
 * buzzer_sound.h
 *
 *  Created on: 10 серп. 2026 р.
 *      Author: anton
 */

#ifndef INC_BUZZER_SOUND_H_
#define INC_BUZZER_SOUND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void buzzer_setup(void);
bool buzzer_is_playing(void);
void buzzer_periodElapsed_callback(void);
void buzzer_play_error(void);
void buzzer_play_success(void);
void buzzer_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_BUZZER_SOUND_H_ */

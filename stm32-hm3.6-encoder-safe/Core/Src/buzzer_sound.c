/*
 * buzzer_sound.c
 *
 *  Created on: 10 серп. 2026 р.
 *      Author: anton
 */

#include "buzzer_sound.h"
#include "main.h"
#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint16_t frequency; // Hz, 0 - pause
    uint16_t duration;  // ms
} Note_t;

// Mario melody
const Note_t success_melody[] = {
    {659, 120}, {0, 30},
    {659, 120}, {0, 150},
    {659, 120}, {0, 150},
    {523, 120}, {0, 30},
    {659, 240}, {0, 60},
    {784, 240}, {0, 300},
    {392, 240}, {0, 300},
};

const Note_t error_melody[] = {
    {311, 100}, {0, 30},
    {262,  250}, {0, 30},
};

static uint16_t timer_step_ms = 10; // default step, get from "get_timer_period_ms" on setup
static uint16_t time_counter_ms = 0;
static uint16_t current_note = 0;
const Note_t * volatile current_melody = NULL;
uint16_t volatile current_melody_length = 0;

uint32_t get_timer_period_ms(TIM_HandleTypeDef *htim) {
    uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
        pclk *= 2;
    }

    uint32_t psc = htim->Instance->PSC;
    uint32_t arr = htim->Instance->ARR;
    uint32_t interrupt_freq_hz = pclk / ((psc + 1) * (arr + 1));

    // convert period to ms: T_ms = 1000 / F_it
    if (interrupt_freq_hz > 0) {
        return 1000 / interrupt_freq_hz;
    }

    return 0;
}

void set_frequency(uint16_t freq) {
    if (freq == 0) {
        __HAL_TIM_SET_COMPARE(&BUZZER_PWD_TIMER, BUZZER_PWD_CHANNEL, 0); // turn off sound
    } else {
        uint32_t arr = (1000000 / freq) - 1;
        __HAL_TIM_SET_AUTORELOAD(&BUZZER_PWD_TIMER, arr);
        __HAL_TIM_SET_COMPARE(&BUZZER_PWD_TIMER, BUZZER_PWD_CHANNEL, arr / 2); // 50% гучності
    }
}

// Запуск програвання з будь-якого місця програми
void play_start(const Note_t *melody, uint16_t length) {
    current_melody = melody;
    current_melody_length = length;
    if (melody == NULL || length == 0) return;

    current_note = 0;
    time_counter_ms = 0;

    set_frequency(current_melody[0].frequency);
    HAL_TIM_PWM_Start(&BUZZER_PWD_TIMER, BUZZER_PWD_CHANNEL);
    HAL_TIM_Base_Start_IT(&BUZZER_TIMER);
}

void buzzer_setup(void) {
    uint32_t period = get_timer_period_ms(&BUZZER_TIMER);
    timer_step_ms = (period > 0) ? period : 10;
}

bool buzzer_is_playing(void) {
    return current_melody != NULL;
}

void buzzer_periodElapsed_callback(void) {
    if (!buzzer_is_playing()) return;

    time_counter_ms += timer_step_ms;

    // Якщо час поточного звуку вичерпано
    if (time_counter_ms >= current_melody[current_note].duration) {
        time_counter_ms = 0;
        current_note++;

        if (current_note < current_melody_length) {
            set_frequency(current_melody[current_note].frequency);
        } else {
            buzzer_stop();
        }
    }
}

void buzzer_play_error(void) {
    if (buzzer_is_playing()) return;

    uint16_t length = sizeof(error_melody) / sizeof(error_melody[0]);
    play_start(error_melody, length);
}

void buzzer_play_success(void) {
    if (buzzer_is_playing()) return;

    uint16_t length = sizeof(success_melody) / sizeof(success_melody[0]);
    play_start(success_melody, length);
}

void buzzer_stop(void) {
    current_melody = NULL;
    current_melody_length = 0;

    HAL_TIM_PWM_Stop(&BUZZER_PWD_TIMER, BUZZER_PWD_CHANNEL);
    HAL_TIM_Base_Stop_IT(&BUZZER_TIMER);
    __HAL_TIM_SET_COMPARE(&BUZZER_PWD_TIMER, BUZZER_PWD_CHANNEL, 0);
}

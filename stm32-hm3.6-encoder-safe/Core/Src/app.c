/*
 * app.c
 *
 *  Created on: 10 серп. 2026 р.
 *      Author: anton
 */


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "app.h"
#include "main.h"
#include "buzzer_sound.h"

#define PIN_LENGTH       4  // 4 values, 0-9
#define MAX_ATTEMPTS     3

const uint8_t SECRET_PIN[PIN_LENGTH] = {3, 7, 0, 5}; // correct PIN

// Event types to handle in "main" loop
typedef enum {
    EVENT_NONE = 0,
    EVENT_SET_NEXT_DIGIT,
    EVENT_SET_NEXT_INDEX,
    EVENT_RESET,
} AppISREvent_t;

uint8_t current_pin[PIN_LENGTH] = {0};
uint8_t attempts_left = MAX_ATTEMPTS;
uint8_t current_digit_index = 0;
int8_t current_digit = -1; // -1: not entered, 0..9

volatile int8_t encoder_direction = 0; // 0: unknown, 1: next (CW), -1: back (CCW)
volatile uint8_t counter_disabled = 0;
volatile AppISREvent_t pending_event = EVENT_NONE;

//-------- Private functions ---------

void lock_system() {
    attempts_left = 0;
    counter_disabled = 1;
    printf("\r\n\r\n=======================");
    printf("\r\n The safe is locked!");
    printf("\r\n=======================\r\n");
    buzzer_play_error();
}

void unlock_system() {
    attempts_left = 0;
    counter_disabled = 1;
    printf("\r\n\r\n=======================");
    printf("\r\n The safe is unlocked!");
    printf("\r\n=======================\r\n");
    buzzer_play_success();
}

void check_pin() {
    bool is_correct = true;
    for (uint8_t i = 0; i < PIN_LENGTH; i++) {
        if (current_pin[i] != SECRET_PIN[i]) {
            is_correct = false;
            break;
        }
    }

    counter_disabled = 1;

    if (is_correct) {
        unlock_system();
    } else {
        printf("\r\nINCORRECT PIN-CODE!\r\n");
        if (attempts_left == 1) {
            lock_system();
        } else {
            buzzer_play_error();
        }
    }
}

void reset() {
    if (attempts_left < 2) return;

    attempts_left--;

    __HAL_TIM_SET_COUNTER(&ENCODER_TIMER, 0);
    encoder_direction = 0;
    current_digit_index = 0;
    current_digit = -1;
    counter_disabled = 0;
    buzzer_stop();

    printf("\r\nAttempts left: %d\r\n", attempts_left);
    printf("Code:  ");
    fflush(stdout);
}

// --- Setup pin-code ----

void set_next_digit() {
    if (current_digit > 8) {
        current_digit = 0;
    } else {
        current_digit++;
    }
    current_pin[current_digit_index] = current_digit;
    printf("\b%d", current_digit);
    fflush(stdout);
}

void set_next_digit_index() {
    if (current_digit_index < PIN_LENGTH-1) {
        current_digit_index++;
        current_digit = 0;
        current_pin[current_digit_index] = current_digit;
        // print space and start value
        printf(" %d", current_digit);
        fflush(stdout);
    } else {
        check_pin();
    }
}

//------- Public functions ------

void app_setup(void) {
    HAL_TIM_Encoder_Start_IT(&ENCODER_TIMER, ENCODER_CHANNEL);
    __HAL_TIM_SET_COUNTER(&ENCODER_TIMER, 0);

    printf("\r\n=== SAFE IS LOCKED ===\r\n");
    printf("Enter PIN-code (%d digits). Attempts left: %d\r\n", PIN_LENGTH, attempts_left);
    printf("Code:  ");
    fflush(stdout);
}

void app_loop(void) {
    if (pending_event == EVENT_NONE) return;

    AppISREvent_t event = pending_event;
    pending_event = EVENT_NONE;

    switch (event) {
        case EVENT_SET_NEXT_DIGIT:
            set_next_digit();
            break;
        case EVENT_SET_NEXT_INDEX:
            set_next_digit_index();
            break;
        case EVENT_RESET:
            reset();
            break;
        default:
            break;
    }
}

void app_isr_counter_callback(void) {
    if (counter_disabled) return;

    static uint32_t last_counter = 0;
    uint32_t current_counter = __HAL_TIM_GET_COUNTER(&ENCODER_TIMER);
    int16_t diff = (int16_t)(current_counter - last_counter);

    if (diff != 0 && diff%2 == 0) {
        int8_t direction = (diff > 0) ? 1 : -1;

        // direction is not initialized
        if (encoder_direction == 0) {
            encoder_direction = direction;
        }

        if (encoder_direction != direction) {
            encoder_direction = direction;
            pending_event = EVENT_SET_NEXT_INDEX;
        } else {
            pending_event = EVENT_SET_NEXT_DIGIT;
        }

        last_counter = current_counter;
    }
}

void app_isr_button_callback(void) {
    static uint32_t last_time = 0;
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_time > 200) {
        last_time = current_time;
        pending_event = EVENT_RESET;
    }
}

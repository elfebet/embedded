/*
 * app.c
 *
 *  Created on: 10 серп. 2026 р.
 *      Author: anton
 */


#include "app.h"
#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define PIN_LENGTH       4  // 4 values, 0-9
#define MAX_ATTEMPTS     3

const uint8_t SECRET_PIN[PIN_LENGTH] = {3, 7, 0, 5}; // correct PIN

uint8_t current_pin[PIN_LENGTH] = {0};
uint8_t attempts_left = MAX_ATTEMPTS;
uint8_t counter_disabled = 0;

uint8_t current_digit_index = 0;
int8_t current_digit = -1; // -1: not entered, 0..9
int8_t encoder_direction = 0; // 0: unknown, 1: next (CW), -1: back (CCW)

//------------------------

void check_pin();
void lock_system();
void unlock_system();
void reset();

//------------------------

// --- Setup pin code ----

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

//------------------------

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
        }
    }
}

void lock_system() {
    attempts_left = 0;
    counter_disabled = 1;
    printf("\r\n\r\n=======================");
    printf("\r\n The safe is locked!");
    printf("\r\n=======================\r\n");
}

void unlock_system() {
    attempts_left = 0;
    counter_disabled = 1;
    printf("\r\n\r\n=======================");
    printf("\r\n The safe is unlocked!");
    printf("\r\n=======================\r\n");
}

void reset() {
    if (attempts_left < 2) return;

    attempts_left--;

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    encoder_direction = 0;
    current_digit_index = 0;
    current_digit = -1;
    counter_disabled = 0;

    printf("\r\nAttempts left: %d\r\n", attempts_left);
    printf("Code:  ");
    fflush(stdout);
}

//------------------------

void app_setup(void) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    printf("\r\n=== SAFE IS LOCKED ===\r\n");
    printf("Enter PIN-code (%d digits). Attempts left: %d\r\n", PIN_LENGTH, attempts_left);
    printf("Code:  ");
    fflush(stdout);
}

void app_counter_callback(void) {
    if (counter_disabled) return;

    static int last_counter = 0;
    int current_counter = __HAL_TIM_GET_COUNTER(&htim2);
    int diff = current_counter - last_counter;

    if (diff != 0 && diff%2 == 0) {
        int8_t direction = (diff > 0) ? 1 : -1;

        // direction is not initialized
        if (encoder_direction == 0) {
            encoder_direction = direction;
        }

        if (encoder_direction != direction) {
            encoder_direction = direction;
            set_next_digit_index();
        } else {
            set_next_digit();
        }
        last_counter = current_counter;
    }
}

void app_button_callback(void) {
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_interrupt_time > 200) {
        last_interrupt_time = current_time;
        reset();
    }
}


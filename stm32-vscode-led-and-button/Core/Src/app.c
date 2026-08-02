#include "app.h"
#include "main.h" // Для доступу до визначень HAL та Pin/Port макросів
#include "stm32f4xx_hal_gpio.h"
#include <stdio.h>
#include <stdbool.h>

#define BTN_POLLING_INTERVAL 10 // ms
#define BTN_DEBOUNCE_TIME 50 // ms

uint32_t led_time = 0;
uint32_t btn_last_debounce_time = 0;
uint32_t btn_last_pool_time = 0;
bool btn_last_state = false;
bool btn_state = false;

void setup(void)
{
    printf("App setup\r\n");
}

void loop_led_blink(void)
{
    uint32_t now = HAL_GetTick();
    if (now - led_time < 2000) return; // 2000 ms have not passed yet

    led_time = now;
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    printf("Led blink. time: %u\r\n", led_time);
}

void loop_button_polling(void)
{
    uint32_t now = HAL_GetTick();
    if (now - btn_last_pool_time < BTN_POLLING_INTERVAL) return; // Not time to poll yet

    btn_last_pool_time = now;
    bool reading = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET;
    if (reading != btn_last_state) {
        btn_last_debounce_time = now;
    }

    if (now - btn_last_debounce_time > BTN_DEBOUNCE_TIME && reading != btn_state) {
        btn_state = reading;
        if (btn_state) {
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
            printf("Button pressed\r\n");
        } else {
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
            printf("Button released\r\n");
        }
    }

    btn_last_state = reading;
}

void loop(void)
{
    loop_led_blink();
    loop_button_polling();
}
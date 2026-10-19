#include "logger.h"
#include "config.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_BUF_LENGTH 64U

void logger(const char *format, ...) {
#if LCD_ENABLE_UART_LOGS
    char text[LOG_BUF_LENGTH];

    va_list args;
    va_start(args, format);
    vsnprintf(text, LOG_BUF_LENGTH - 3, format, args);
    va_end(args);
    strncat(text, "\r\n", 3);

    HAL_UART_Transmit(&huart1, (uint8_t*)text, strlen(text), HAL_MAX_DELAY);
#endif
}

void logger_reset_timer() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable DWT
    DWT->CYCCNT = 0;                                // Clear counter
    DWT->CTRL = DWT_CTRL_CYCCNTENA_Msk;             // Enable counter
}

uint32_t logger_start_time() {
    return DWT->CYCCNT;
}

uint32_t logger_elapsed_us(uint32_t start_time) {
    return (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000000U) / SystemCoreClock);
}

uint32_t logger_elapsed_ms(uint32_t start_time) {
    return (uint32_t)(((uint64_t)(DWT->CYCCNT - start_time) * 1000U) / SystemCoreClock);
}

uint32_t cycles_to_us(uint32_t cycles) {
    return (uint32_t)((uint64_t)cycles * 1000000U / SystemCoreClock);
}

uint32_t cycles_to_ms(uint32_t cycles) {
    return (uint32_t)((uint64_t)cycles * 1000U / SystemCoreClock);
}

#include "logger.h"
#include "config.h"
#include "main.h"
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

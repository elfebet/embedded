#include "u8g2_hal.h"
#include "main.h"   // extern I2C_HandleTypeDef hi2c1;
#include <string.h>
#include <stdio.h>

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32]; // Buffer for I2C data
    static uint8_t buf_idx;

    switch (msg) {
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;

        case U8X8_MSG_BYTE_SEND:
            // Copy incoming bytes to local buffer
            memcpy(&buffer[buf_idx], arg_ptr, arg_int);
            buf_idx += arg_int;
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 100);
            if (status != HAL_OK) {
                printf("HAL_I2C_Master_Transmit failed: %d", status);
                return 0; // Failure
            }
            break;

        case U8X8_MSG_BYTE_INIT:
        default:
            break;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            break;

        case U8X8_MSG_DELAY_10MICRO: // 10us delay
          for (uint16_t i = 0; i < 84; i++) __NOP(); // Rough estimate for 84MHz clock
          break;

        case U8X8_MSG_DELAY_100NANO: // 100ns delay
          __NOP();
          break;

        case U8X8_MSG_GPIO_AND_DELAY_INIT:
        case U8X8_MSG_GPIO_RESET: // reset-піна в SSD1306-модулі з I2C зазвичай немає
        default:
            break;
    }
    return 1;
}

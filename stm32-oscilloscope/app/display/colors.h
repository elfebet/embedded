#pragma once
#include <stdint.h>

// Colors in RGB565 format (16bit)
#define COLOR_WHITE       0xFFFF
#define COLOR_BLACK       0x0000
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_BROWN       0x9260
#define COLOR_ORANGE      0xFC00
#define COLOR_PURPLE      0x8010
#define COLOR_MAGENTA     0xF81F
#define COLOR_CYAN        0x07FF
#define COLOR_GRAY        0x8410

static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

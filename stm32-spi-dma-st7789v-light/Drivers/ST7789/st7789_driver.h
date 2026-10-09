#pragma once
#include <stdbool.h>
#include <stdint.h>

void st7789_init(uint8_t orientation);
void st7789_backlight(bool on);

void st7789_rotation(uint8_t orientation);
void st7789_invertColors(bool invert);
void st7789_tearEffect(bool on);
void st7789_setPower(bool on);
void st7789_sleepIn(bool in);

void st7789_begin(void);
void st7789_end(void);

void st7789_setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7789_writeData(const uint8_t *data, uint32_t len);

#pragma once
#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} st7789_pin_t;

typedef struct {
    SPI_HandleTypeDef *spi;
    st7789_pin_t cs;
    st7789_pin_t dc;
    st7789_pin_t rst;
    st7789_pin_t blk;
} st7789_spi_connected_data_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    int16_t x_offset;
    int16_t y_offset;
    uint8_t orientation;
} st7789_window_data_t;

void st7789_init(const st7789_spi_connected_data_t *spi_data, const st7789_window_data_t *window_data);
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7789_backlight(bool on);

void st7789_invert_colors(bool invert);
void st7789_tear_effect(bool on);
void st7789_set_power(bool on);
void st7789_sleep_in(bool in);

uint16_t st7789_width(void);
uint16_t st7789_height(void);

void st7789_fill_pixels(uint32_t count, uint16_t color);
void st7789_draw_pixel(uint16_t color);

void st7789_write_pixels(const uint16_t *pixels, uint32_t count);
void st7789_write_pixels_blocking(const uint16_t *pixels, uint32_t count);

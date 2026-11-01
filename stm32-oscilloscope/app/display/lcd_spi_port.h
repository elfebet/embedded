#pragma once
#include "stm32f4xx_hal.h"

typedef enum {
    LCD_SPI_MEM_INCREMENT = 0,  // draw image, frame buffer, font?
    LCD_SPI_MEM_DMA_FIXED = 1,  // ONLY FOR DMA, fill window / rect / line
} lcd_spi_mem_t;

typedef enum {
    LCD_SPI_SIZE_8BIT  = 0, // commands, draw 1 pixel, 8-bit frame buffer
    LCD_SPI_SIZE_16BIT = 1, // image, fill rect, fill v/h lines, 16-bit frame buffer, font?
} lcd_spi_size_t;

void lcd_spi_init(SPI_HandleTypeDef *hspi);

void lcd_delay(uint32_t ms);
uint8_t lcd_spi_dma_ready();
void lcd_spi_dma_wait(void);

// Transmit `count` SPI data units of the current size
// 8-bit: count is bytes
// 16-bit: count is half-words

// The blocking function reads consecutive units from `data`.
void lcd_spi_transmit_blocking(const uint8_t *data, uint32_t count, lcd_spi_size_t size);

// DMA uses `mem` to either advance through consecutive units or repeatedly read the first unit.
// With LCD_SPI_MEM_DMA_FIXED, `data` must point to one unit (e.g. one 16-bit color).
void lcd_spi_transmit_dma(const uint8_t *data, uint32_t count, lcd_spi_size_t size, lcd_spi_mem_t mem);

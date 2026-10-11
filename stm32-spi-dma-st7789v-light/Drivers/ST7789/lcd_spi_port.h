#pragma once
#include "stm32f4xx_hal.h"

typedef enum {
    LCD_SPI_MEM_INCREMENT = 0,  // draw image, frame buffer, font?
    LCD_SPI_MEM_FIXED = 1,     // fill window / rect / line
} lcd_spi_mem_t;

typedef enum {
    LCD_SPI_SIZE_8BIT  = 0, // commands, draw 1 pixel, 8-bit frame buffer
    LCD_SPI_SIZE_16BIT = 1, // image, fill rect, fill v/h lines, 16-bit frame buffer, font?
} lcd_spi_size_t;

void lcd_spi_init(SPI_HandleTypeDef *hspi);

// SPI data frame format: 8 or 16 bit
void lcd_spi_set_size(lcd_spi_size_t size);

// SPI DMA config. Also calls lcd_spi_set_size(size)
void lcd_spi_set_dma(lcd_spi_mem_t mem, lcd_spi_size_t size);

void lcd_delay(uint32_t ms);
uint8_t lcd_spi_dma_ready();
void lcd_spi_dma_wait(void);

// Transmit `count` SPI data units of the current size
// 8-bit: count is bytes
// 16-bit: count is half-words
void lcd_spi_transmit_blocking(const uint8_t *data, uint32_t count);
void lcd_spi_transmit_dma(const uint8_t *data, uint32_t count);

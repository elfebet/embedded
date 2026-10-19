#pragma once

#define LCD_USE_DMA          1
#define LCD_ENABLE_TESTS     1
#define LCD_ENABLE_UART_LOGS 1

#if !LCD_USE_DMA
// Blocking fallback buffer capacity in bytes for fixed-source transfers.
#define LCD_SPI_FIXED_BUFFER_SIZE_BYTES 256U
#endif

#define GFX_MAX_BUFFER_FONT_WIDTH  20U
#define GFX_MAX_BUFFER_FONT_HEIGHT 28U

#define ADC_DMA_BUFFER_SIZE        2048U // units for uint16_t
#define ADC_MIN_SAMPLE_RATE_HZ     10U
#define ADC_MAX_SAMPLE_RATE_HZ     500000U
#define ADC_DEFAULT_SAMPLE_RATE_HZ 1000U

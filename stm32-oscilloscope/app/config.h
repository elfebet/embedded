#pragma once

#define LCD_USE_DMA      1
#define LCD_ENABLE_TESTS 1

#if !LCD_USE_DMA
// Blocking fallback buffer capacity in bytes for fixed-source transfers.
#define LCD_SPI_FIXED_BUFFER_SIZE_BYTES 256
#endif

#define GFX_MAX_BUFFER_FONT_WIDTH  20
#define GFX_MAX_BUFFER_FONT_HEIGHT 28

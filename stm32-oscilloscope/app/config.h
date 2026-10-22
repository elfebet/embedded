#pragma once

#define LCD_USE_DMA          1
#define LCD_ENABLE_TESTS     1
#define LCD_ENABLE_UART_LOGS 1

#if !LCD_USE_DMA
// Blocking fallback buffer capacity in bytes for fixed-source transfers.
#define LCD_SPI_FIXED_BUFFER_SIZE_BYTES 256U
#endif

// Maximum buffer size for font rendering in the graphics library
#define GFX_MAX_BUFFER_FONT_WIDTH  20U
#define GFX_MAX_BUFFER_FONT_HEIGHT 28U

// ADC configuration parameters
#define ADC_MAX_CODE               4095U
#define ADC_DMA_BUFFER_SIZE        2048U // units for uint16_t
#define ADC_MIN_SAMPLE_RATE_HZ     10U
#define ADC_MAX_SAMPLE_RATE_HZ     500000U
#define ADC_DEFAULT_SAMPLE_RATE_HZ 1000U

/*
=========== USB ===========
Vin         V3V3 (pin 8)	Vpin7
0              3305         1584
+3312          3305         0623
−3312          3305         2549
+5044          3305         0119
−5044          3305         3056
*/
#define VDDA_MV             3305.0f // яка напруга на живлені від USB (не використовується)
#define FRONTEND_ZERO_MV    1584.0f // яка напруга коли SIGNAL в 0V (GND). Vpin7 at Vin = 0 when VDDA = 3306 mV
#define FRONTEND_ZERO_RATIO 0.480f  // FRONTEND_ZERO_MV / VDDA_MV. Vpin7 / VDDA when Vin = 0,
// ADC and frontend configuration parameters
// Як рахуємо. Беремо min / max занчення на вході і визначаємо коефіцієнт підсилення фронтенду
// Приклад 1
// При +3,3V (3312) на вході Vin, Vpin7 = 0623 mV
// При −3,3V (-3312) на вході Vin, Vpin7 = 2549 mV
// GAIN = (2549 - 0623) / 6624 (різниця входу) = 0.290
// Приклад 2
// При +5V ( 5044) на вході Vin, Vpin7 = 0117 mV
// При −5V (-5044) на вході Vin, Vpin7 = 3056 mV
// GAIN = (3056 - 119) / 10088 (різниця входу) = 0.291
#define FRONTEND_GAIN       0.291f  // |dVpin7 / dVin|, output is inverted

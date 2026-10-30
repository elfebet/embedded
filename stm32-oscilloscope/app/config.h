#pragma once

#define LCD_USE_DMA          1
#define LCD_ENABLE_TESTS     1

#if !LCD_USE_DMA
// Blocking fallback buffer capacity in bytes for fixed-source transfers.
#define LCD_SPI_FIXED_BUFFER_SIZE_BYTES 256U
#endif

// Maximum buffer size for font rendering in the graphics library
#define GFX_MAX_BUFFER_FONT_WIDTH  20U
#define GFX_MAX_BUFFER_FONT_HEIGHT 28U

#define APP_TARGET_FPS       120
#define APP_ENABLE_UART_LOGS 1
#define APP_SPLASH_SCREEN    APP_RELEASE
#define APP_SHOW_WAVES_MODE  0 // display or not waves mode: grouped or normal (GRP/NRM)

// Inputs (buttons, encoders, etc.)
#define APP_INPUT_MAX_BUTTONS   8U
#define APP_INPUT_MAX_ENCODERS  4U

// ADC configuration parameters
#define ADC_MAX_CODE          4095U
#define ADC_DMA_BUFFER_SIZE   4096U // units for uint16_t
#define ADC_TRIGGER_SCAN_SIZE 3072U // 3/4 of ADC_DMA_BUFFER_SIZE. Larger than the ADC_SNAPSHOT_SIZE to allow pre-trigger sampling
#define ADC_SNAPSHOT_SIZE     1000U
#define ADC_GRID_H_DIVS       10U
#define ADC_GRID_V_DIVS       8U

#define ADC_MIN_SAMPLE_RATE_HZ       10U
#define ADC_MAX_SAMPLE_RATE_HZ       500000U
#define ADC_PREFERRED_SAMPLE_RATE_HZ 5000U
#define SCOPE_INPUT_MAX_MV           5000U

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

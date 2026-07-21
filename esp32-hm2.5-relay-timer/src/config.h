#pragma once

#ifdef ESP32_ESPRESSIF_BOARD
    #define FAN_PIN GPIO_NUM_4
#else // arduino board
    #define FAN_PIN 4
#endif

// set time in microseconds (1 second = 1 000 000 microseconds)
#define FAN_RUN_TIME_US     (5 * 1000 * 1000) // 5 seconds
#define FAN_OFF_TIME_US     (10 * 1000 * 1000) // 10 seconds
// #define FAN_RUN_TIME_US     (15ULL * 60 * 1000 * 1000) // 15 minutes
// #define FAN_OFF_TIME_US     (45ULL * 60 * 1000 * 1000) // 45 minutes (60 min - 15 min)
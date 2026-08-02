#pragma once

#define LDR_LED_PIN         GPIO_NUM_4
#define LDR_ADC_UNIT        ADC_UNIT_1
#define LDR_ADC_CHANNEL     ADC_CHANNEL_5 // ADC1_5 -> GPIO 6
#define LDR_THRESH_DARK     2000 // less than this value — dark (LED ON)
#define LDR_THRESH_LIGHT    2400 // greater than this value — light (LED OFF)

#define NTC_ADC_UNIT        ADC_UNIT_2
#define NTC_ADC_CHANNEL     ADC_CHANNEL_6  // ADC2_6 -> GPIO 17

#define ADC_ATTEN       ADC_ATTEN_DB_12 // (0 - ~3.3V)
#define ADC_BITWIDTH    ADC_BITWIDTH_12 // 12bit resolution (0..4095)
#define ADC_MAX_VAL     4095.0f   // 12bit ADC (0..4095)

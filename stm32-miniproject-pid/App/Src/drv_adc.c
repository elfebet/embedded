#include "drv_adc.h"

uint16_t adc_read_mv(const adc_t *a) {
    uint32_t sum = 0, num = 0;
    for (uint8_t i = 0; i < a->oversample; i++) {
        HAL_ADC_Start(a->hadc);
        if (HAL_ADC_PollForConversion(a->hadc, 1) == HAL_OK) {
            sum += HAL_ADC_GetValue(a->hadc); // values: 0..4095
            num++;
        }
    }

    if (num == 0) return 0;

    uint16_t avg = sum / num;
    return (uint16_t)((avg * a->vref_mv) / 4095u);
}

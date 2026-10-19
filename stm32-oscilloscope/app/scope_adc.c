#include "scope_adc.h"
#include "config.h"

static volatile uint16_t s_adc_buffer[ADC_DMA_BUFFER_SIZE];
static uint32_t s_sample_rate_hz = 0;
static scope_adc_data_t s_adc_data;

static HAL_StatusTypeDef get_sample_timer_period(uint32_t rate_hz, uint32_t *period) {
    if (rate_hz < ADC_MIN_SAMPLE_RATE_HZ || rate_hz > ADC_MAX_SAMPLE_RATE_HZ || period == NULL) return HAL_ERROR;

    uint32_t flash_latency = 0;
    RCC_ClkInitTypeDef clock_config = {0};
    HAL_RCC_GetClockConfig(&clock_config, &flash_latency);
    uint32_t timer_clock_hz = HAL_RCC_GetPCLK1Freq();
    if (clock_config.APB1CLKDivider != RCC_HCLK_DIV1) {
        timer_clock_hz *= 2U;
    }

    *period = (timer_clock_hz + rate_hz / 2U) / rate_hz;
    return HAL_OK;
}

static void set_sample_timer(uint32_t timer_period) {
    s_adc_data.tim->Init.Prescaler = 0;
    s_adc_data.tim->Init.Period = timer_period - 1;
    __HAL_TIM_SET_PRESCALER(s_adc_data.tim, 0);
    __HAL_TIM_SET_AUTORELOAD(s_adc_data.tim, timer_period - 1);
    s_adc_data.tim->Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(s_adc_data.tim, TIM_FLAG_UPDATE);
    __HAL_TIM_SET_COUNTER(s_adc_data.tim, 0);
}

static void clear_buffer() {
    for (uint16_t i = 0; i < ADC_DMA_BUFFER_SIZE; ++i) {
        s_adc_buffer[i] = 2048;
    }
}

void scope_adc_init(const scope_adc_data_t *scope_adc_data, uint32_t rate_hz) {
    s_adc_data = *scope_adc_data;
    // set DMA ContinuousRequests = ENABLED
    s_adc_data.adc->Instance->CR2 |= ADC_CR2_DDS;
    scope_adc_set_sample_rate(rate_hz);
}

HAL_StatusTypeDef scope_adc_start(void) {
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(s_adc_data.adc, (uint32_t *)s_adc_buffer, ADC_DMA_BUFFER_SIZE);
    if (status != HAL_OK) return status;

    status = HAL_TIM_Base_Start(s_adc_data.tim);
    if (status != HAL_OK) {
        HAL_ADC_Stop_DMA(s_adc_data.adc);
        return status;
    }

    return HAL_OK;
}

HAL_StatusTypeDef scope_adc_stop(void) {
    HAL_StatusTypeDef status = HAL_TIM_Base_Stop(s_adc_data.tim);
    if (status != HAL_OK) return status;

    status = HAL_ADC_Stop_DMA(s_adc_data.adc);
    if (status != HAL_OK) return status;

    return HAL_OK;
}

HAL_StatusTypeDef scope_adc_copy_latest(uint16_t *destination, uint32_t sample_count) {
    if (destination == NULL || sample_count == 0U || sample_count >= ADC_DMA_BUFFER_SIZE) return HAL_ERROR;

    const uint32_t dma_remaining = __HAL_DMA_GET_COUNTER(s_adc_data.adc->DMA_Handle);
    if (dma_remaining > ADC_DMA_BUFFER_SIZE) return HAL_ERROR;

    const uint32_t write_index = (ADC_DMA_BUFFER_SIZE - dma_remaining) % ADC_DMA_BUFFER_SIZE;
    const uint32_t first_sample = (write_index + ADC_DMA_BUFFER_SIZE - sample_count) % ADC_DMA_BUFFER_SIZE;

    for (uint32_t i = 0U; i < sample_count; ++i) {
        destination[i] = s_adc_buffer[(first_sample + i) % ADC_DMA_BUFFER_SIZE];
    }

    return HAL_OK;
}

uint32_t scope_adc_sample_rate(void) {
    return s_sample_rate_hz;
}

HAL_StatusTypeDef scope_adc_set_sample_rate(uint32_t rate_hz) {
    if (rate_hz == s_sample_rate_hz) return HAL_OK;

    uint32_t timer_period;
    HAL_StatusTypeDef status = get_sample_timer_period(rate_hz, &timer_period);
    if (status != HAL_OK) return status;

    const uint8_t was_running = (s_adc_data.tim->Instance->CR1 & TIM_CR1_CEN) != 0;
    if (was_running) {
        status = scope_adc_stop();
        if (status != HAL_OK) return status;
    }

    set_sample_timer(timer_period);
    clear_buffer();

    if (was_running) {
        status = scope_adc_start();
        if (status != HAL_OK) return status;
    }

    s_sample_rate_hz = rate_hz;
    return HAL_OK;
}

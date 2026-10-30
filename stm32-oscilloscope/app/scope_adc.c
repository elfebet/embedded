#include "scope_adc.h"
#include "config.h"
#include "stm32f4xx_ll_adc.h"

static volatile uint16_t s_adc_buffer[ADC_DMA_BUFFER_SIZE];
static uint32_t s_sample_rate_hz = 0;
static scope_adc_data_t s_adc_data;
static volatile uint32_t s_dma_wraps;
static uint32_t s_total_samples;

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

// Called by HAL each time the circular DMA buffer wraps around.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc == s_adc_data.adc) ++s_dma_wraps;
}

void scope_adc_init(const scope_adc_data_t *scope_adc_data, uint32_t rate_hz) {
    s_adc_data = *scope_adc_data;
    // set DMA ContinuousRequests = ENABLED
    s_adc_data.adc->Instance->CR2 |= ADC_CR2_DDS;
    scope_adc_set_sample_rate(rate_hz);
}

HAL_StatusTypeDef scope_adc_start(void) {
    clear_buffer();
    s_total_samples = 0;
    s_dma_wraps = 0;
    __HAL_TIM_SET_COUNTER(s_adc_data.tim, 0);
    __HAL_TIM_CLEAR_FLAG(s_adc_data.tim, TIM_FLAG_UPDATE);

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

bool scope_adc_is_running(void) {
    return (s_adc_data.tim->Instance->CR1 & TIM_CR1_CEN) != 0;
}

HAL_StatusTypeDef scope_adc_copy_latest(uint16_t *destination, uint32_t sample_count, uint32_t *total_samples) {
    if (destination == NULL || sample_count == 0U || sample_count >= ADC_DMA_BUFFER_SIZE) return HAL_ERROR;

    uint32_t wraps;
    uint32_t dma_remaining;
    do {
        wraps = s_dma_wraps;
        dma_remaining = __HAL_DMA_GET_COUNTER(s_adc_data.adc->DMA_Handle);
    } while (wraps != s_dma_wraps);
    if (dma_remaining > ADC_DMA_BUFFER_SIZE) return HAL_ERROR;

    const uint32_t write_index = (ADC_DMA_BUFFER_SIZE - dma_remaining) % ADC_DMA_BUFFER_SIZE;
    if (total_samples != NULL) {
        const uint32_t total = wraps * ADC_DMA_BUFFER_SIZE + write_index;
        // keep the counter monotonic if the wrap interrupt is not serviced yet
        if ((int32_t)(total - s_total_samples) > 0) s_total_samples = total;
        *total_samples = s_total_samples;
    }

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

    const uint8_t was_running = scope_adc_is_running();
    if (was_running) {
        status = scope_adc_stop();
        if (status != HAL_OK) return status;
    }

    set_sample_timer(timer_period);
    if (was_running) {
        status = scope_adc_start();
        if (status != HAL_OK) return status;
    }

    s_sample_rate_hz = rate_hz;
    return HAL_OK;
}

HAL_StatusTypeDef scope_adc_measure_vdda_mv(uint32_t *vdda_mv) {
    if (vdda_mv == NULL || s_sample_rate_hz == 0 ||
        (s_adc_data.tim->Instance->CR1 & TIM_CR1_CEN) != 0 ||
        (s_adc_data.adc->Instance->CR2 & ADC_CR2_DMA) != 0) {
        return HAL_ERROR;
    }

    const uint32_t trigger_edges = s_adc_data.adc->Instance->CR2 & ADC_CR2_EXTEN;
    ADC_ChannelConfTypeDef channel_config = {
        .Channel = ADC_CHANNEL_VREFINT,
        .Rank = 1,
        .SamplingTime = ADC_SAMPLETIME_480CYCLES,
    };
    HAL_StatusTypeDef status = HAL_ADC_ConfigChannel(s_adc_data.adc, &channel_config);
    uint32_t raw_sum = 0;
    const uint32_t measurement_count = 16;

    if (status == HAL_OK) {
        MODIFY_REG(s_adc_data.adc->Instance->CR2, ADC_CR2_EXTEN, 0);
        HAL_Delay(1);

        for (uint32_t i = 0; i < measurement_count; ++i) {
            status = HAL_ADC_Start(s_adc_data.adc);
            if (status != HAL_OK) {
                break;
            }

            status = HAL_ADC_PollForConversion(s_adc_data.adc, 10);
            if (status == HAL_OK) {
                raw_sum += HAL_ADC_GetValue(s_adc_data.adc);
            }

            const HAL_StatusTypeDef stop_status = HAL_ADC_Stop(s_adc_data.adc);
            if (status == HAL_OK && stop_status != HAL_OK) {
                status = stop_status;
            }
            if (status != HAL_OK) {
                break;
            }
        }
    }

    const HAL_StatusTypeDef stop_status = HAL_ADC_Stop(s_adc_data.adc);
    if (status == HAL_OK && stop_status != HAL_OK) {
        status = stop_status;
    }

    channel_config.Channel = ADC_CHANNEL_0;
    channel_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    const HAL_StatusTypeDef restore_status = HAL_ADC_ConfigChannel(s_adc_data.adc, &channel_config);
    MODIFY_REG(s_adc_data.adc->Instance->CR2, ADC_CR2_EXTEN, trigger_edges);

    if (status != HAL_OK) return status;
    if (restore_status != HAL_OK) return restore_status;

    const uint32_t vrefint_raw = (raw_sum + measurement_count / 2) / measurement_count;
    if (vrefint_raw == 0) return HAL_ERROR;

    *vdda_mv = ((uint32_t)(*VREFINT_CAL_ADDR) * VREFINT_CAL_VREF + vrefint_raw / 2) / vrefint_raw;
    return HAL_OK;
}

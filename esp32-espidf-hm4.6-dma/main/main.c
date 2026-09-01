#include <stdio.h>
#include <stdbool.h>
#include <sys/_types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "soc/clk_tree_defs.h"

#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_3      // ADC1_3 -> GPIO4
#define ADC_ATTEN       ADC_ATTEN_DB_12    // full range ~0-3.3В
#define ADC_BIT_WIDTH   ADC_BITWIDTH_12

#define SAMPLE_FREQ_HZ  20000 // 20 kHz
#define FRAME_SIZE      256   // байтів на один "готовий блок" -- аналог половини Circular-буфера на STM32
#define POOL_FRAMES     4     // скільки таких блоків тримати в кільці одночасно

#define UART_PORT       UART_NUM_0
#define UART_BAUD       115200
#define UART_TX_BUF     1024

typedef struct {
    uint32_t min_raw;
    uint32_t max_raw;
    uint32_t sum_raw;
    uint32_t total_samples;
} adc_stats_t;

static adc_continuous_handle_t adc_handle;
static TaskHandle_t adc_task_handle;

static bool IRAM_ATTR adc_conv_done_cb(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata,
    void *user_data
) {
    BaseType_t must_yield = pdFALSE;
    vTaskNotifyGiveFromISR(adc_task_handle, &must_yield);
    return must_yield == pdTRUE;
}

static void process_and_send(adc_digi_output_data_t *data, int count, adc_stats_t *stats) {
    for (int i = 0; i < count; i++) {
        uint32_t raw = data[i].type2.data;
        if (raw < stats->min_raw) stats->min_raw = raw;
        if (raw > stats->max_raw) stats->max_raw = raw;

        stats->sum_raw += raw;
        stats->total_samples++;
    }

    // send per 0.5 sec (20000 Hz it's 20000/2 counts)
    if (stats->total_samples >= SAMPLE_FREQ_HZ/2) {
        static const float vref = 3.3f;
        static const float adc_resolution = 4095.0f;
        unsigned long avg_raw = stats->sum_raw / stats->total_samples;
        float v_min = ((float)stats->min_raw / adc_resolution) * vref;
        float v_max = ((float)stats->max_raw / adc_resolution) * vref;
        float v_avg = ((float)avg_raw / adc_resolution) * vref;
        
        char tx_buf[80];
        int tx_len = snprintf(tx_buf, sizeof(tx_buf),
            "Samples: %lu | Vmin: %.2fV | Vmax: %.2fV | Vpp: %.2fV | Vavg: %.2fV\r\n",
            (unsigned long)stats->total_samples, v_min, v_max, v_max - v_min, v_avg);

        // неблокуюче -- копіює в кільцевий буфер драйвера й повертається
        uart_write_bytes(UART_PORT, tx_buf, tx_len);

        // reset data for next 1 sec interval
        stats->min_raw = UINT32_MAX;
        stats->max_raw = 0;
        stats->sum_raw = 0;
        stats->total_samples = 0;
    }
}

static void adc_read_task(void *arg) {
    uint8_t result[FRAME_SIZE] = {0};
    uint32_t bytes_read = 0;
    adc_stats_t stats = {
        .min_raw = UINT32_MAX,
        .max_raw = 0,
        .sum_raw = 0,
        .total_samples = 0,
    };

    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // заснути, поки не прийде сигнал з adc_conv_done_cb

        esp_err_t err = adc_continuous_read(
            adc_handle, 
            result, 
            FRAME_SIZE, 
            &bytes_read, 
            100
        );
        if (err == ESP_OK) {
            process_and_send(
                (adc_digi_output_data_t *)result, 
                bytes_read / sizeof(adc_digi_output_data_t),
                &stats
            );
        }
    }
}

void adc_dma_setup(void) {
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = FRAME_SIZE * POOL_FRAMES, // сумарний розмір кільця DMA-буферів
        .conv_frame_size = FRAME_SIZE,                  // розмір ОДНОГО готового блоку -- аналог dma_buf_len
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));

    adc_digi_pattern_config_t pattern[1] = {
        {
            .atten = ADC_ATTEN,
            .channel = ADC_CHANNEL,
            .unit = ADC_UNIT,
            .bit_width = ADC_BIT_WIDTH,
        },
    };
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_FREQ_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .pattern_num = 1,
        .adc_pattern = pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));

    adc_continuous_evt_cbs_t cbs = { .on_conv_done = adc_conv_done_cb };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
}

void uart_setup(void) {
    uart_config_t config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));

    ESP_ERROR_CHECK(uart_driver_install(
        UART_PORT, 
        256, // виділяємо мін буфер, що перевищує розмір FIFO (128 байт). 0 нельзя, буде помилка
        UART_TX_BUF,  // enable buffered, non-blocked transmission
        0, 
        NULL, 
        0
    ));
}

void app_main(void) {
    uart_setup();
    adc_dma_setup();

    xTaskCreate(
        adc_read_task, 
        "adc_read_task", 
        4096, 
        NULL, 
        5, 
        &adc_task_handle
    );
}

#include "app.h"
#include "colors.h"
#include "config.h"
#include "fonts.h"
#include "gfx.h"
#include "gfx_text.h"
#include "main.h"
#include "logger.h"
#include "scope_renderer.h"
#include "st7789_driver.h"
#include "scope_adc.h"
#include "stm32f4xx_hal.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TARGET_FPS 120
#define STATUS_HEIGHT 18
#define ADC_SNAPSHOT_SIZE  512U

static uint16_t adc_snapshot[ADC_SNAPSHOT_SIZE];

static void draw_status(uint32_t fps) {
    char status_text[24];
    snprintf(status_text, sizeof(status_text), "FPS:%lu SR:%luHz", fps, scope_adc_sample_rate());
    gfx_fill_rect(0, 0, st7789_width() - 1, STATUS_HEIGHT - 1, COLOR_BLACK);
    gfx_draw_text(0, 0, status_text, &Font_7x10, COLOR_WHITE, COLOR_BLACK);
}

void display_init(void) {
    st7789_spi_connected_data_t spi_data = {
        .spi = &hspi1,
        .dc = {TFT_DC_GPIO_Port, TFT_DC_Pin},
        .cs = {TFT_CS_GPIO_Port, TFT_CS_Pin},
        .rst = {TFT_RST_GPIO_Port, TFT_RST_Pin},
        .blk = {TFT_BLK_GPIO_Port, TFT_BLK_Pin},
    };
    st7789_window_data_t window_data = {
        .width = 320,
        .height = 240,
        .orientation = GFX_ORIENTATION_LANDSCAPE,
    };

    st7789_init(&spi_data, &window_data);
}

void adc_init() {
    scope_adc_data_t adc_data = {
        .adc = &hadc1,
        .tim = &htim2,
    };
    scope_adc_init(&adc_data, ADC_DEFAULT_SAMPLE_RATE_HZ);
}

void app_start(void) {
    adc_init();
    display_init();

    if (ADC_SNAPSHOT_SIZE >= ADC_DMA_BUFFER_SIZE) {
        logger("ADC_SNAPSHOT_SIZE must be less than ADC_DMA_BUFFER_SIZE");
        Error_Handler();
    }

    scope_renderer_fill_background();
    scope_renderer_draw_grid();
    if (scope_adc_start() != HAL_OK) {
        logger("Failed to start adc");
        Error_Handler();
    }

    // To limit the rendering time to TARGET_FPS
    uint32_t last_frame_cycles = logger_get_time(); // for calculate curent FPS
    const uint32_t frame_interval_cycles = SystemCoreClock / TARGET_FPS; // preferred fPS

    // FPS
    uint32_t current_fps = 0;
    uint32_t frames_in_window = 0; // for measuring and displaying FPS
    uint32_t fps_window_start_cycles = logger_get_time(); // time interval for displaying the FPS label: once per second
    uint64_t wave_render_cycles = 0;

    while (1) {
        const uint32_t now_cycles = logger_get_time();
        if (now_cycles - last_frame_cycles < frame_interval_cycles) {
            continue;
        }
        last_frame_cycles = now_cycles;

        const HAL_StatusTypeDef copy_status = scope_adc_copy_latest(adc_snapshot, ADC_SNAPSHOT_SIZE);
        if (copy_status != HAL_OK) {
            logger("Failed to copy adc buffer");
            Error_Handler();
        }

        uint32_t stage_start_cycles = logger_get_time();
        scope_renderer_draw_wave(adc_snapshot, ADC_SNAPSHOT_SIZE, false);
        wave_render_cycles += logger_get_time() - stage_start_cycles;

        ++frames_in_window;

        const uint32_t fps_now_cycles = logger_get_time();
        const uint32_t elapsed_cycles = fps_now_cycles - fps_window_start_cycles;
        if (elapsed_cycles >= SystemCoreClock) {
            current_fps = (uint32_t)(((uint64_t)frames_in_window * SystemCoreClock + elapsed_cycles / 2) / elapsed_cycles);
            draw_status(current_fps);

            const uint32_t avg_wave_redner_us = cycles_to_us(wave_render_cycles / frames_in_window);
            logger("FPS:%lu Render:%lu us", current_fps, avg_wave_redner_us);

            frames_in_window = 0;
            wave_render_cycles = 0;
            fps_window_start_cycles = fps_now_cycles;
        }
    }
}

#include "app.h"
#include "colors.h"
#include "config.h"
#include "fonts.h"
#include "gfx.h"
#include "gfx_text.h"
#include "input_events.h"
#include "main.h"
#include "logger.h"
#include "st7789_driver.h"
#include "scope_adc.h"
#include "scope_renderer.h"
#include "app_controls.h"
#include "app_input.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "dwt_timer.h"
#include "app_state.h"

#define TARGET_FPS 120
#define STATUS_HEIGHT 24

#define ADC_SNAPSHOT_SIZE  512U
static uint16_t adc_snapshot[ADC_SNAPSHOT_SIZE];
static app_state_t app_state;

static void draw_status(uint32_t adc_avg) {
    char status_text[32];
    const float pin7_mv = (float)adc_avg * (float)app_state.adc_vdda_mv / ADC_MAX_CODE;
    const float vin_mv = (FRONTEND_ZERO_RATIO * (float)app_state.adc_vdda_mv - pin7_mv) / FRONTEND_GAIN;
    const int32_t vin = (int32_t)(vin_mv >= 0.0f ? vin_mv + 0.5f : vin_mv - 0.5f);
    const int32_t vin_abs = vin < 0 ? -vin : vin;
    snprintf(status_text, sizeof(status_text), "ADC:%lu P7:%ldmV Vin:%c%ld.%03ldV",
             adc_avg, (long)pin7_mv, vin < 0 ? '-' : '+', (long)(vin_abs / 1000), (long)(vin_abs % 1000));
    gfx_draw_text(0, 20, status_text, &Font_7x10, COLOR_RED, COLOR_BLACK);
}

static uint32_t adc_average(const uint16_t *samples, uint32_t count) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; ++i) {
        sum += samples[i];
    }
    return sum / count;
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
    scope_adc_init(&adc_data, app_state.sample_rate_hz);

    if (scope_adc_measure_vdda_mv(&app_state.adc_vdda_mv) != HAL_OK) {
        logger("Failed to measure VDDA using Vrefint");
        Error_Handler();
    }
    logger("Measured VDDA: %lumV", app_state.adc_vdda_mv);
    scope_renderer_set_vdda_mv(app_state.adc_vdda_mv);

    if (ADC_SNAPSHOT_SIZE >= ADC_DMA_BUFFER_SIZE) {
        logger("ADC_SNAPSHOT_SIZE must be less than ADC_DMA_BUFFER_SIZE");
        Error_Handler();
    }
}

void controls_init(void) {
    const button_input_data_t buttons[] = {
        { ENC_SW1_GPIO_Port, ENC_SW1_Pin, INPUT_ACTION_ENC_BTN_1, INPUT_ACTION_ENC_BTN_1_LONG_PRESS },
        { ENC_SW2_GPIO_Port, ENC_SW2_Pin, INPUT_ACTION_ENC_BTN_2, INPUT_ACTION_ENC_BTN_2_LONG_PRESS },
        { BTN_1_GPIO_Port,   BTN_1_Pin,   INPUT_ACTION_BTN_1, INPUT_ACTION_BTN_1_LONG_PRESS },
        { BTN_2_GPIO_Port,   BTN_2_Pin,   INPUT_ACTION_BTN_2, INPUT_ACTION_BTN_2_LONG_PRESS },
        { BTN_3_GPIO_Port,   BTN_3_Pin,   INPUT_ACTION_BTN_3, INPUT_ACTION_BTN_3_LONG_PRESS },
        { BTN_4_GPIO_Port,   BTN_4_Pin,   INPUT_ACTION_BTN_4, INPUT_ACTION_BTN_4_LONG_PRESS },
    };

    const uint32_t num_buttons = sizeof(buttons) / sizeof(buttons[0]);
    if (app_input_buttons_init(buttons, num_buttons) != HAL_OK) {
        logger("Failed to init buttons");
        Error_Handler();
    }

    const encoder_input_data_t s_encoders[] = {
        { &htim3, INPUT_VALUE_HORIZONTAL_KNOB },
        { &htim4, INPUT_VALUE_VERTICAL_KNOB },
    };
    const uint32_t num_encoders = sizeof(s_encoders) / sizeof(s_encoders[0]);
    if (app_input_encoders_init(s_encoders, num_encoders) != HAL_OK) {
        logger("Failed to init encoders");
        Error_Handler();
    }
}

void app_start(void) {
    app_state_init(&app_state);
    display_init();
    controls_init();
    adc_init();

    scope_renderer_fill_background();
    scope_renderer_draw_grid();
    if (scope_adc_start() != HAL_OK) {
        logger("Failed to start adc");
        Error_Handler();
    }

    // To limit the rendering time to TARGET_FPS
    uint32_t last_frame_cycles = dwt_get_time(); // for calculate curent FPS
    const uint32_t frame_interval_cycles = SystemCoreClock / TARGET_FPS; // preferred fPS

    // FPS
    uint32_t frames_in_window = 0; // for measuring and displaying FPS
    uint32_t fps_window_start_cycles = dwt_get_time(); // time interval for displaying the FPS label: once per second
    uint64_t wave_render_cycles = 0;

    while (1) {
        const uint32_t now_cycles = dwt_get_time();
        if (now_cycles - last_frame_cycles < frame_interval_cycles) {
            continue;
        }
        last_frame_cycles = now_cycles;

        input_events_t input_events;
        app_input_poll();
        app_input_take_events(&input_events);

        // const vertical_knob_mode_t previous_vertical_mode = settings.vertical_knob_mode;
        app_controls_handle(&app_state, &input_events);

        // if (settings.vertical_knob_mode != previous_vertical_mode) {
        //     draw_status(adc_average(adc_snapshot, ADC_SNAPSHOT_SIZE));
        // }

        scope_renderer_draw_display_mode(&app_state);
        scope_renderer_draw_sample_rate(&app_state);


        if (app_state.running) {
            const HAL_StatusTypeDef copy_status = scope_adc_copy_latest(adc_snapshot, ADC_SNAPSHOT_SIZE);
            if (copy_status != HAL_OK) {
                logger("Failed to copy adc buffer");
                Error_Handler();
            }

            uint32_t stage_start_cycles = dwt_get_time();
            scope_renderer_draw_wave_grouped(adc_snapshot, ADC_SNAPSHOT_SIZE, app_state.display_mode);
            wave_render_cycles += dwt_get_time() - stage_start_cycles;
        }

        ++frames_in_window;

        const uint32_t fps_now_cycles = dwt_get_time();
        const uint32_t elapsed_cycles = fps_now_cycles - fps_window_start_cycles;
        if (elapsed_cycles >= SystemCoreClock) {
            app_state.fps = (uint32_t)(((uint64_t)frames_in_window * SystemCoreClock + elapsed_cycles / 2) / elapsed_cycles);
            scope_renderer_draw_fps(&app_state);
            draw_status(adc_average(adc_snapshot, ADC_SNAPSHOT_SIZE));

            const uint32_t avg_wave_redner_us = dwt_cycles_to_us(wave_render_cycles / frames_in_window);
            logger("FPS:%lu Render:%lu us", app_state.fps, avg_wave_redner_us);

            frames_in_window = 0;
            wave_render_cycles = 0;
            fps_window_start_cycles = fps_now_cycles;
        }

        app_state.needs_update = 0;
    }
}

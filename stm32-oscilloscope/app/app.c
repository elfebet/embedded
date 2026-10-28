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
#include "app_profiler.h"
#include "scope_stats.h"

static uint16_t adc_snapshot[ADC_SNAPSHOT_SIZE];
static app_state_t app_state;
static adc_metrics_t adc_metrics;
static app_profiler_stats_t profiler_stats;

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

    if (ADC_SNAPSHOT_SIZE >= ADC_DMA_BUFFER_SIZE) {
        logger("ADC_SNAPSHOT_SIZE must be less than ADC_DMA_BUFFER_SIZE");
        Error_Handler();
    }
}

void controls_init(void) {
    const button_input_data_t buttons[] = {
        { BTN_1_GPIO_Port,   BTN_1_Pin,   INPUT_ACTION_TRIGGER_MODE_NEXT },
        { BTN_2_GPIO_Port,   BTN_2_Pin,   INPUT_ACTION_TRIGGER_EDGE_NEXT, INPUT_ACTION_TRIGGER_POSITION_NEXT },
        { BTN_3_GPIO_Port,   BTN_3_Pin,   INPUT_ACTION_RUN_STOP_TOGGLE, INPUT_ACTION_RESET },
        { BTN_4_GPIO_Port,   BTN_4_Pin,   INPUT_ACTION_DISPLAY_MODE_NEXT, INPUT_ACTION_WAVES_DRAW_MODE_NEXT },
        { ENC_SW1_GPIO_Port, ENC_SW1_Pin, INPUT_ACTION_VERTICAL_KNOB_MODE, INPUT_ACTION_RESET_VERTICAL_POSITION },
        { ENC_SW2_GPIO_Port, ENC_SW2_Pin, INPUT_ACTION_HORIZONTAL_KNOB_MODE, INPUT_ACTION_RESET_TRIGGER_LEVEL },
    };

    const uint32_t num_buttons = sizeof(buttons) / sizeof(buttons[0]);
    if (app_input_buttons_init(buttons, num_buttons) != HAL_OK) {
        logger("Failed to init buttons");
        Error_Handler();
    }

    const encoder_input_data_t s_encoders[] = {
        { &htim3, INPUT_VALUE_VERTICAL_KNOB },
        { &htim4, INPUT_VALUE_HORIZONTAL_KNOB },
    };
    const uint32_t num_encoders = sizeof(s_encoders) / sizeof(s_encoders[0]);
    if (app_input_encoders_init(s_encoders, num_encoders) != HAL_OK) {
        logger("Failed to init encoders");
        Error_Handler();
    }
}

void update_sample_rate_if_needed(void) {
    if (app_state.sample_rate_hz == scope_adc_sample_rate()) return;

    if (scope_adc_set_sample_rate(app_state.sample_rate_hz) != HAL_OK) {
        logger("Failed to set sample rate");
        Error_Handler();
    }
}

void app_start(void) {
    app_state_init(&app_state);
    display_init();
    controls_init();
    adc_init();

    scope_stats_reinit(app_state.adc_vdda_mv);
    scope_renderer_reinit(app_state.adc_vdda_mv);
    if (scope_adc_start() != HAL_OK) {
        logger("Failed to start adc");
        Error_Handler();
    }

    // To limit the rendering time to TARGET_FPS
    uint32_t last_frame_cycles = dwt_get_time();
    const uint32_t frame_interval_cycles = SystemCoreClock / TARGET_FPS;

    app_profiler_init();
    while (1) {
        const uint32_t now_cycles = dwt_get_time();
        if (now_cycles - last_frame_cycles < frame_interval_cycles) {
            continue;
        }
        last_frame_cycles = now_cycles;

        input_events_t input_events;
        app_input_poll();
        app_input_take_events(&input_events);

        app_controls_handle(&app_state, &input_events);
        update_sample_rate_if_needed();

        scope_renderer_draw_run_stop(&app_state);
        const bool display_mode_changed = scope_renderer_draw_display_mode(&app_state);
        const bool waves_mode_changed = scope_renderer_draw_waves_mode(&app_state);
        scope_renderer_draw_time_div(&app_state);
        scope_renderer_draw_sample_rate(&app_state);
        scope_renderer_draw_vertical(&app_state);
        const bool vertical_changed = scope_renderer_update_vertical(&app_state);

        if (app_state.running || vertical_changed || display_mode_changed || waves_mode_changed) {
            if (app_state.running) {
                const HAL_StatusTypeDef copy_status = scope_adc_copy_latest(adc_snapshot, ADC_SNAPSHOT_SIZE);
                if (copy_status != HAL_OK) {
                    logger("Failed to copy adc buffer");
                    Error_Handler();
                }
            }

            if (app_state.grouped_waves_draw_mode != app_state.last_grouped_waves_draw_mode) {
                scope_renderer_erase_wave();
                scope_renderer_erase_wave_grouped();
                app_state.last_grouped_waves_draw_mode = app_state.grouped_waves_draw_mode;
            }

            app_profiler_wave_render_begin();
            if (app_state.grouped_waves_draw_mode) {
                scope_renderer_draw_wave_grouped(adc_snapshot, ADC_SNAPSHOT_SIZE, app_state.display_mode);
            } else {
                scope_renderer_draw_wave(adc_snapshot, ADC_SNAPSHOT_SIZE, app_state.display_mode);
            }
            app_profiler_wave_render_end();

            if (scope_stats_update_periodic(adc_snapshot, ADC_SNAPSHOT_SIZE, &adc_metrics)) {
                scope_renderer_draw_adc_vdda(&adc_metrics);
            }
        }

        app_profiler_frame_end();
        if (app_profiler_take_stats(&profiler_stats)) {
            scope_renderer_draw_fps(profiler_stats.fps);
            logger("FPS:%lu Render:%lu us", profiler_stats.fps, profiler_stats.avg_wave_render_us);
        }

        app_state.needs_update = 0;
    }
}

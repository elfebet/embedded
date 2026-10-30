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
#include "scope_renderer_labels.h"
#include "app_controls.h"
#include "app_input.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dwt_timer.h"
#include "app_state.h"
#include "profiler.h"
#include "scope_stats.h"
#include "scope_capture.h"
#include <string.h>

static uint16_t adc_scan[ADC_TRIGGER_SCAN_SIZE];
static app_state_t app_state;
static adc_metrics_t adc_metrics;
static signal_metrics_t signal_metrics;
static profiler_stats_t profiler_stats;
static scope_capture_result_t capture_result;

static scope_wave_display_mode_t last_displays_mode;
static bool last_grouped_waves_mode;

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

void splash_screen(void) {
#if APP_SPLASH_SCREEN
    gfx_fill_screen(COLOR_BLACK);

    const char *message = "Oscilloscope";
    const size_t length = strlen(message);
    const font_def_t *font = &Font_16x26;
    char str_buf[2];
    str_buf[1] = '\0';
    uint16_t x_pos = (st7789_width() - length * font->width) / 2;
    uint16_t y_pos = st7789_height()/2 - Font_16x26.height;
    for (int i = 0; i < length; i++) {
        str_buf[0] = message[i];
        gfx_draw_text(x_pos, y_pos, str_buf, font, COLOR_WHITE, COLOR_BLACK);
        HAL_Delay(50);
        x_pos += font->width;
    }

    HAL_Delay(2000);
#endif
}

void reset_if_needed(void) {
    if (!app_state.reset_requested) return;

    app_state_reset(&app_state);
    scope_capture_init(adc_scan);
    scope_renderer_reinit(app_state.adc_vdda_mv);
    scope_renderer_labels_clear_buffer();
}

void start_stop_adc_if_needed(void) {
    if (app_state.running == scope_adc_is_running()) return;

    if (app_state.running) {
        if (scope_adc_start() != HAL_OK) {
            logger("Failed to start adc");
            Error_Handler();
        }
    } else {
        if (scope_adc_stop() != HAL_OK) {
            logger("Failed to stop adc");
            Error_Handler();
        }
    }
}

static void sync_adc_with_app_state(void) {
    if (!app_state.running) {
        start_stop_adc_if_needed();
    }

    if (app_state.sample_rate_hz != scope_adc_sample_rate()) {
        scope_stats_set_sample_rate(app_state.sample_rate_hz);
        if (scope_adc_set_sample_rate(app_state.sample_rate_hz) != HAL_OK) {
            logger("Failed to set sample rate");
            Error_Handler();
        }
    }

    if (app_state.running) {
        start_stop_adc_if_needed();
    }
}

bool should_update_waves() {
    if (last_displays_mode != app_state.display_mode
        || last_grouped_waves_mode != app_state.grouped_waves_draw_mode) {

        last_displays_mode = app_state.display_mode;
        last_grouped_waves_mode = app_state.grouped_waves_draw_mode;
        return true;
    }
    return false;
}

void app_start(void) {
    app_state_init(&app_state);
    display_init();
    controls_init();
    adc_init();
    splash_screen();

    last_displays_mode = app_state.display_mode;
    last_grouped_waves_mode = app_state.grouped_waves_draw_mode;
    scope_stats_reinit(app_state.adc_vdda_mv);
    scope_stats_set_sample_rate(app_state.sample_rate_hz);

    scope_capture_init(adc_scan);
    scope_renderer_reinit(app_state.adc_vdda_mv);
    scope_renderer_labels_clear_buffer();
    start_stop_adc_if_needed();

    // To limit the rendering time to APP_TARGET_FPS
    uint32_t last_frame_cycles = dwt_get_time();
    const uint32_t frame_interval_cycles = SystemCoreClock / APP_TARGET_FPS;

    profiler_init();
    while (1) {
        const uint32_t now_cycles = dwt_get_time();
        if (now_cycles - last_frame_cycles < frame_interval_cycles) {
            continue;
        }
        last_frame_cycles = now_cycles;

        input_events_t input_events;
        app_input_poll();
        app_input_take_events(&input_events);
        if (app_controls_handle(&app_state, &input_events)) {
            reset_if_needed();
            sync_adc_with_app_state();
        }

        scope_renderer_draw_top_labels(&app_state);
        const bool vertical_changed = scope_renderer_update_vertical(&app_state);
        const bool update_waves = should_update_waves();

        uint32_t total_samples = 0;
        if (app_state.running) {
            if (scope_adc_copy_latest(adc_scan, ADC_TRIGGER_SCAN_SIZE, &total_samples) != HAL_OK) {
                logger("Failed to copy adc buffer");
                Error_Handler();
            }
        }

        scope_capture_update(&capture_result, &app_state, total_samples);
        scope_renderer_draw_capture_state(capture_result.state);
        if (capture_result.single_completed_update) {
            app_state.running = false;
            start_stop_adc_if_needed();
            scope_renderer_draw_run_stop(&app_state);
        }

        if (capture_result.new_frame_ready || vertical_changed || update_waves) {
            if (app_state.grouped_waves_draw_mode != app_state.last_grouped_waves_draw_mode) {
                scope_renderer_erase_wave();
                scope_renderer_erase_wave_grouped();
                app_state.last_grouped_waves_draw_mode = app_state.grouped_waves_draw_mode;
            }

            profiler_wave_render_begin();
            if (app_state.grouped_waves_draw_mode) {
                scope_renderer_draw_wave_grouped(capture_result.samples, app_state.display_mode);
            } else {
                scope_renderer_draw_wave(capture_result.samples, app_state.display_mode);
            }
            profiler_wave_render_end();

            if (capture_result.single_completed_update || app_state.needs_update) {
                scope_stats_calc_metrics(capture_result.samples, &adc_metrics, &signal_metrics);
                scope_renderer_draw_bottom_labels(&adc_metrics, &signal_metrics);
            } else {
               if (scope_stats_update_periodic(capture_result.samples, &adc_metrics, &signal_metrics)) {
                    scope_renderer_draw_bottom_labels(&adc_metrics, &signal_metrics);
                }
            }
        }

        profiler_frame_end();
        if (profiler_take_stats(&profiler_stats)) {
            scope_renderer_draw_waves_mode(&app_state);
            scope_renderer_draw_fps(profiler_stats.fps);
            logger("FPS:%lu Render:%luus",
                profiler_stats.fps,
                profiler_stats.avg_wave_render_us);
        }

        app_state.needs_update = 0;
    }
}

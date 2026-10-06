#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h" 

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#define LCD_H_RES 240
#define LCD_V_RES 320
#define LCD_BUFFER_SIZE        (LCD_H_RES * LCD_V_RES)

#define LCD_PIN_SCLK           7   // SCL clock signal
#define LCD_PIN_MOSI           6   // SDA data
#define LCD_PIN_RST            5   // RST reset

#define LCD_PIN_DC             12    // Data/Command (RS)
#define LCD_PIN_CS             11    // Chip Select
#define LCD_PIN_BL             10    // Backlighting


// BOUNCING BALL
#define BALL_SIZE              20
#define BALL_BUFFER_SIZE       (BALL_SIZE * BALL_SIZE)
#define COLOR_BACKGROUND       0x0000 // black
#define COLOR_BALL             0xF800 // red
#define COMBINED_ZONE_SIZE     40

static const char *TAG = "TFT_ST7789V";

/*
void app_main(void) {
    ESP_LOGI(TAG, "Init BL (backlighting) display");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_BL
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(LCD_PIN_BL, 1);

    ESP_LOGI(TAG, "Init SPI");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_SCLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_BUFFER_SIZE  * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Init Panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = 1 * 1000 * 1000, // 1 MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Init ST7789V driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI(TAG, "Display initialized. Start fill colors...");

    uint16_t *frame_buffer = heap_caps_malloc(LCD_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (frame_buffer == NULL) {
        ESP_LOGE(TAG, "Failed malloc color_buffer!");
        return;
    }

    // RGB565 colors
    const uint16_t colors[] = {
        0xF800, // red
        0x07E0, // green
        0x001F, // blue
        0xFFFF, // white
        0x0000, // black
    };
    const uint8_t num_colors = sizeof(colors) / sizeof(colors[0]);

    int color_index = 0;
    int frame_count = 0;
    int64_t last_time = esp_timer_get_time();

    while (1) {
        uint16_t current_color = colors[color_index];

        for (int i = 0; i < LCD_BUFFER_SIZE; i++) {
            frame_buffer[i] = current_color;
        }
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, frame_buffer);

        frame_count++;
        color_index = (color_index + 1) % num_colors;

        int64_t now = esp_timer_get_time();
        if (now - last_time >= 1000000) {
            float fps = (float)frame_count * 1000000.0f / (now - last_time);
            ESP_LOGI(TAG, "FPS: %.2f", fps);

            frame_count = 0;
            last_time = now;
        }
    }

    free(frame_buffer);
}
*/

void app_main(void) {
    ESP_LOGI(TAG, "Init BL (backlighting) display");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_BL
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(LCD_PIN_BL, 1);

    ESP_LOGI(TAG, "Init SPI");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_SCLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Init Panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = 40 * 1000 * 1000, // 40 MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Init ST7789V driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI(TAG, "Display initialized. Start fill colors...");

    // clear buffer with black color (only once at start)
    uint16_t *fullscreen = heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (fullscreen) {
        for(int i=0; i < LCD_H_RES*LCD_V_RES; i++) {
            fullscreen[i] = COLOR_BACKGROUND;
        }
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, fullscreen);
        free(fullscreen);
        vTaskDelay(pdMS_TO_TICKS(100));
    } else {
        ESP_LOGE(TAG, "Failed to allocate DMA memory for fullscreen!");
    }

//    uint16_t *ball_buffer = heap_caps_malloc(BALL_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
//    uint16_t *clear_buffer = heap_caps_malloc(BALL_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
    uint16_t *combined_buffer = heap_caps_malloc(COMBINED_ZONE_SIZE * COMBINED_ZONE_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);

    // ball coordinates
    int ball_x = 100;
    int ball_y = 150;
    int ball_dx = 1; // X speed (pixed per frame)
    int ball_dy = 1; // Y speed
    int prev_x = ball_x;
    int prev_y = ball_y;

    const int TARGET_FPS = 120;
    const int64_t TARGET_FRAME_TIME_US = 1000000 / TARGET_FPS; // 16666 мікросекунд на кадр

    int64_t last_frame_time = esp_timer_get_time();
    int64_t fps_timer = esp_timer_get_time();
    int frame_count = 0;

    ESP_LOGI(TAG, "Start game...");

    while (1) {
        int64_t now = esp_timer_get_time();

        if (now - last_frame_time >= TARGET_FRAME_TIME_US) {
            last_frame_time = now; // Запам'ятовуємо час старту кадру

            prev_x = ball_x;
            prev_y = ball_y;

            ball_x += ball_dx;
            ball_y += ball_dy;

            if (ball_x <= 0 || ball_x >= (LCD_H_RES - BALL_SIZE)) {
                ball_dx = -ball_dx;
                ball_x += ball_dx;
            }
            if (ball_y <= 0 || ball_y >= (LCD_V_RES - BALL_SIZE)) {
                ball_dy = -ball_dy;
                ball_y += ball_dy;
            }

            // 1. Знаходимо межі загального прямокутника (Bounding Box), який об'єднує старе та нове положення
             int min_x = (ball_x < prev_x) ? ball_x : prev_x;
             int max_x = ((ball_x > prev_x) ? ball_x : prev_x) + BALL_SIZE;
             int min_y = (ball_y < prev_y) ? ball_y : prev_y;
             int max_y = ((ball_y > prev_y) ? ball_y : prev_y) + BALL_SIZE;

             int zone_w = max_x - min_x;
             int zone_h = max_y - min_y;

             // Перевірка безпеки буфера (якщо зона раптом більша за виділені 40х40)
             if (zone_w > COMBINED_ZONE_SIZE) zone_w = COMBINED_ZONE_SIZE;
             if (zone_h > COMBINED_ZONE_SIZE) zone_h = COMBINED_ZONE_SIZE;

             // 2. Очищаємо спільну робочу зону кольором фону прямо в RAM
             for (int i = 0; i < zone_w * zone_h; i++) {
                 combined_buffer[i] = COLOR_BACKGROUND;
             }

             // 3. Малюємо новий м'ячик поверх фону в нашому локальному буфері
             // Вираховуємо локальні координати м'ячика відносно початку нашої спільної зони (min_x, min_y)
             int local_ball_x = ball_x - min_x;
             int local_ball_y = ball_y - min_y;

             for (int y = 0; y < BALL_SIZE; y++) {
                 for (int x = 0; x < BALL_SIZE; x++) {
                     int buffer_index = (local_ball_y + y) * zone_w + (local_ball_x + x);
                     combined_buffer[buffer_index] = COLOR_BALL;
                 }
             }

             esp_lcd_panel_draw_bitmap(
                panel_handle, 
                min_x, 
                min_y, 
                min_x + zone_w, 
                min_y + zone_h, 
                combined_buffer
            );
            
            frame_count++;
        }

        if (now - fps_timer >= 1000000) {
            float fps = (float)frame_count * 1000000.0f / (now - fps_timer);
            ESP_LOGI(TAG, "FPS: %.2f", fps);
            frame_count = 0;
            fps_timer = now;
        }
    }
}

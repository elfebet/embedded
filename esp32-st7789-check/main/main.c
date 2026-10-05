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

static const char *TAG = "TFT_ST7789V";


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

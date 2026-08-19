#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"

#define BUTTON_PIN  4
#define LED_PIN     6
#define TXD_PIN     17
#define RXD_PIN     18

#define UART_PORT           UART_NUM_1
#define UART_BAUD_RATE      9600
#define UART_BUF_SIZE       256
#define BTN_DEBOUNCE_DELAY  50

#define CMD_LED_ON  '+'
#define CMD_LED_OFF '-'

static const char *TAG = "UART";

void setup(void) {
    // configure LED
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&led_conf));

    // configure BUTTON
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&btn_conf));

    // configure UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(
        UART_PORT, 
        &uart_config
    ));
    ESP_ERROR_CHECK(uart_set_pin(
        UART_PORT, 
        TXD_PIN, 
        RXD_PIN, 
        UART_PIN_NO_CHANGE, 
        UART_PIN_NO_CHANGE
    ));
    ESP_ERROR_CHECK(uart_driver_install(
        UART_PORT, 
        UART_BUF_SIZE, 
        UART_BUF_SIZE,
        0, 
        NULL, 
        0
    ));
}

void app_main(void)
{
    setup();

    int last_btn_state = 0;
    uint32_t last_debounce_time = 0;
    uint8_t rx_byte = 0;
    gpio_set_level(LED_PIN, 0);

     while (true) {
         int32_t now = esp_timer_get_time() / 1000;

         // handle button state and send data via uart
         int btn_state = gpio_get_level(BUTTON_PIN);
         if (btn_state != last_btn_state) {
             if ((now - last_debounce_time) > BTN_DEBOUNCE_DELAY) {
                 last_debounce_time = now;
                 const uint8_t cmd = btn_state == 1 ? CMD_LED_ON : CMD_LED_OFF;
                 ESP_LOGI(TAG, "Send uart: %c", cmd);
                 uart_write_bytes(UART_PORT, &cmd, 1);
                 last_btn_state = btn_state;
             }
         }

         // read uart bytes and switch LED state
         uint32_t wait = pdMS_TO_TICKS(10);
         int len = uart_read_bytes(UART_PORT, &rx_byte, sizeof(rx_byte), wait);
         if (len > 0) {
            ESP_LOGI(TAG, "Got: 0x%02X ('%c')", rx_byte, rx_byte);
            if (CMD_LED_ON == rx_byte) {
                gpio_set_level(LED_PIN, 1);
            } else if (CMD_LED_OFF == rx_byte) {
                gpio_set_level(LED_PIN, 0);
            }
         }
     }
}

#include <stdio.h>
#include "env_spi.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define REG_ID         0xD0
#define REG_CTRL_HUM   0xF2
#define REG_CTRL_MEAS  0xF4
#define REG_PRESS_MSB  0xF7
#define SPI_HOST_USED  SPI3_HOST

static const char *TAG = "ENV_SPI";
static spi_device_handle_t bme;
static bme280_calib_t calib = {0};

static esp_err_t bme280_read_regs(uint8_t reg, uint8_t *out, size_t len) {
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    tx[0] = reg | 0x80; // read-bit
    memset(&tx[1], 0x00, len);

    spi_transaction_t t = {
        .length = 8 * (1 + len),
        .tx_buffer = tx,
        .rx_buffer = rx
    };
    esp_err_t err = spi_device_polling_transmit(bme, &t);
    if (err == ESP_OK) memcpy(out, &rx[1], len);
    return err;
}

static esp_err_t bme280_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), value }; // write-bit = 0
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx
    };
    return spi_device_polling_transmit(bme, &t);
}

void env_spi_init(int mosi_pin, int miso_pin, int sclk_pin, int cs_pin) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_USED, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,                         // CPOL=0, CPHA=0 - required for BME280
        .spics_io_num = cs_pin,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_USED, &devcfg, &bme));

    uint8_t chip_id = 0;
    ESP_ERROR_CHECK(bme280_read_regs(REG_ID, &chip_id, 1));
    ESP_LOGI(TAG, "SPI BME280 id=0x%02X (очікували 0x60)", chip_id);

    // reset + delay
    bme280_write_reg(0xE0, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t buf_tp[26], buf_h[7];
    ESP_ERROR_CHECK(bme280_read_regs(0x88, buf_tp, sizeof(buf_tp)));
    ESP_ERROR_CHECK(bme280_read_regs(0xE1, buf_h, sizeof(buf_h)));
    bme280_parse_calib(&calib, buf_tp, buf_h);

    ESP_LOGI(TAG, "SPI ready: SCLK=%d MOSI=%d MISO=%d CS=%d", sclk_pin, mosi_pin, miso_pin, cs_pin);
}

bool env_spi_read(bme280_data_t *out) {
    if (bme280_write_reg(REG_CTRL_HUM, 0x01) != ESP_OK)
        return false; // osrs_h=x1

    uint8_t ctrl_meas = (1 << 5) | (1 << 2) | 0x01; // forced mode
    if (bme280_write_reg(REG_CTRL_MEAS, ctrl_meas) != ESP_OK)
        return false;
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t raw[8];
    if (bme280_read_regs(REG_PRESS_MSB, raw, sizeof(raw)) != ESP_OK)
        return false;

    bme280_read_measurements(&calib, raw, out);
    return true;
}
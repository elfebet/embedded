#include "bme280.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include <unistd.h>

static const char *TAG = "SPI_bme280";
static spi_device_handle_t bme280;

void bme280_spi_bus_and_device_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1, // не використовується в Single SPI -- явно "немає такого піна"
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(
        SPI_HOST_USED, 
        &buscfg,
        SPI_DMA_CH_AUTO
    ));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,  // 1 МГц -- багато нижче за максимум BME280 (10 МГц) і VSPI (~26 МГц), свідомо "з запасом" для першого запуску
        .mode = 0,                          // CPOL=0, CPHA=0 -- Mode 0, обов'язковий для BME280
        .spics_io_num = PIN_CS,             // драйвер сам опускає/піднімає цей пін навколо кожної транзакції
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_USED, &devcfg, &bme280));

    ESP_LOGI(TAG, "SPI3(VSPI) ready: SCLK=%d MOSI=%d MISO=%d CS=%d", PIN_SCLK, PIN_MOSI, PIN_MISO, PIN_CS);
}

void bme280_check_device(void) {
    uint8_t tx[2] = { BME280_REG_ID, 0x00 }; // байт 0: адреса+read-біт; байт 1: dummy, генерує ще 8 тактів
    uint8_t rx[2] = { 0 };

    spi_transaction_t t = {
        .length = 8 * sizeof(tx),   // довжина в БІТАХ, не байтах -- 2 байти = 16 біт
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_polling_transmit(bme280, &t);

    if (err == ESP_OK) {
        uint8_t chip_id = rx[1]; // rx[0] -- службовий байт під час передачі адреси, ігнорується
        ESP_LOGI(TAG, "Chip ID = 0x%02X (очікували 0x60)%s", chip_id,
                 chip_id == 0x60 ? "  -- OK" : "  -- НЕ ЗБІГАЄТЬСЯ, перевірте підключення/режим");
    } else {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(err));
    }
}
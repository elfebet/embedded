#include "bme280.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <unistd.h>

#define SPI_HOST_USED   SPI3_HOST      // VSPI, через GPIO matrix, до ~26 МГц
#define BME280_REG_ID 0xD0           // "адреса" з уже встановленим read-бітом (0x50 | 0x80)
#define BME280_REG_CTRL_HUM   0xF2
#define BME280_REG_CTRL_MEAS  0xF4
#define BME280_REG_PRESS_MSB  0xF7 // 0xF7-0xFE: press(3B), temp(3B), hum(2B) -- один суцільний блок, burst-читається одним викликом

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static const char *TAG = "SPI_bme280";
static spi_device_handle_t bme280;
static bme280_calib_t calib;
static double t_fine;   // "спільна" проміжна величина

void bme280_spi_bus_and_device_init(int mosi_pin, int miso_pin, int sclk_pin, int cs_pin) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = sclk_pin,
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
        .spics_io_num = cs_pin,             // драйвер сам опускає/піднімає цей пін навколо кожної транзакції
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_USED, &devcfg, &bme280));

    ESP_LOGI(TAG, "SPI ready: SCLK=%d MOSI=%d MISO=%d CS=%d", sclk_pin, mosi_pin, miso_pin, cs_pin);
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

esp_err_t bme280_read_regs(uint8_t reg, uint8_t *out, size_t len) {
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    tx[0] = reg | 0x80;         // set read-bit
    memset(&tx[1], 0x00, len);  // dummy-байти для решти тактів

    spi_transaction_t t = {
        .length = 8 * (1 + len),
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_polling_transmit(bme280, &t);
    if (err == ESP_OK) memcpy(out, &rx[1], len);   // rx[0] -- службовий байт, відкидаємо
    return err;
}

esp_err_t bme280_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), value }; // set write-bit = 0
    spi_transaction_t t = {
        .length = 2*8,
        .tx_buffer = tx
    };
    return spi_device_polling_transmit(bme280, &t);
}

static uint16_t u16_le(const uint8_t *b) {
    return (uint16_t)(b[0] | (b[1] << 8)); // Little-Endian!
}

void bme280_read_calibration(void) {
    uint8_t buf_tp[26];   // регістри 0x88-0xA1: dig_T1-T3, dig_P1-P9 (+dig_H1 в останньому байті)
    uint8_t buf_h[7];     // регістри 0xE1-0xE7: dig_H2-H6

    ESP_ERROR_CHECK(bme280_read_regs(0x88, buf_tp, sizeof(buf_tp)));
    ESP_ERROR_CHECK(bme280_read_regs(0xE1, buf_h, sizeof(buf_h)));

    calib.dig_T1 = u16_le(&buf_tp[0]);
    calib.dig_T2 = (int16_t)u16_le(&buf_tp[2]);
    calib.dig_T3 = (int16_t)u16_le(&buf_tp[4]);
    calib.dig_P1 = u16_le(&buf_tp[6]);
    calib.dig_P2 = (int16_t)u16_le(&buf_tp[8]);
    calib.dig_P3 = (int16_t)u16_le(&buf_tp[10]);
    calib.dig_P4 = (int16_t)u16_le(&buf_tp[12]);
    calib.dig_P5 = (int16_t)u16_le(&buf_tp[14]);
    calib.dig_P6 = (int16_t)u16_le(&buf_tp[16]);
    calib.dig_P7 = (int16_t)u16_le(&buf_tp[18]);
    calib.dig_P8 = (int16_t)u16_le(&buf_tp[20]);
    calib.dig_P9 = (int16_t)u16_le(&buf_tp[22]);
    calib.dig_H1 = buf_tp[25];

    calib.dig_H2 = (int16_t)u16_le(&buf_h[0]);
    calib.dig_H3 = buf_h[2];
    // dig_H4/dig_H5 -- 12-бітні значення, "розмазані" по трьох напів-байтах (типова економія пам'яті виробником)
    calib.dig_H4 = (int16_t)((buf_h[3] << 4) | (buf_h[4] & 0x0F));
    calib.dig_H5 = (int16_t)((buf_h[5] << 4) | (buf_h[4] >> 4));
    calib.dig_H6 = (int8_t)buf_h[6];

    ESP_LOGI(TAG, "Calibration loaded: dig_T1=%u dig_P1=%u dig_H1=%u", calib.dig_T1, calib.dig_P1, calib.dig_H1);
}

double bme280_compensate_temperature(int32_t adc_T) {
    double var1, var2, T;
    var1 = (((double)adc_T) / 16384.0 - ((double)calib.dig_T1) / 1024.0) * ((double)calib.dig_T2);
    var2 = ((((double)adc_T) / 131072.0 - ((double)calib.dig_T1) / 8192.0) *
            (((double)adc_T) / 131072.0 - ((double)calib.dig_T1) / 8192.0)) * ((double)calib.dig_T3);
    t_fine = var1 + var2;
    T = (var1 + var2) / 5120.0;
    return T;   // градуси Цельсія
}

double bme280_compensate_pressure(int32_t adc_P) {
    double var1, var2, p;
    var1 = ((double)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)calib.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)calib.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)calib.dig_P4) * 65536.0);
    var1 = (((double)calib.dig_P3) * var1 * var1 / 524288.0 + ((double)calib.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)calib.dig_P1);
    if (var1 == 0.0) return 0;   // захист від ділення на нуль -- фізично неможливо при справному датчику
    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)calib.dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)calib.dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)calib.dig_P7)) / 16.0;
    return p / 100.0;   // Паскалі -> гектопаскалі (hPa), звичні "мм рт. ст."-подібні одиниці метеозведень
}

double bme280_compensate_humidity(int32_t adc_H) {
    double var_H;
    var_H = (((double)t_fine) - 76800.0);
    var_H = (adc_H - (((double)calib.dig_H4) * 64.0 + ((double)calib.dig_H5) / 16384.0 * var_H)) *
            (((double)calib.dig_H2) / 65536.0 * (1.0 + ((double)calib.dig_H6) / 67108864.0 * var_H *
            (1.0 + ((double)calib.dig_H3) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((double)calib.dig_H1) * var_H / 524288.0);
    if (var_H > 100.0) var_H = 100.0;        // фізична межа -- відносна вологість не може перевищувати 100%
    else if (var_H < 0.0) var_H = 0.0;
    return var_H;   // %RH
}

void bme280_force_measurement(void) {
    // Bosch forced mode: одне вимірювання за викликом, датчик сам повертається в sleep після завершення --
    // економія енергії проти continuous mode, підходить під наш цикл "раз на секунду"
    ESP_ERROR_CHECK(bme280_write_reg(BME280_REG_CTRL_HUM, 0x01));  // oversampling x1 для вологості
    ESP_ERROR_CHECK(bme280_write_reg(BME280_REG_CTRL_MEAS, 0x25)); // temp x1, press x1, mode=forced(01)
    vTaskDelay(pdMS_TO_TICKS(10)); // час на власне вимірювання -- датчик сам "прокидається", рахує, засинає
}

void bme280_read_measurements(bme280_data_t *data) {
    uint8_t raw[8];
    ESP_ERROR_CHECK(bme280_read_regs(BME280_REG_PRESS_MSB, raw, sizeof(raw)));

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);   // 20-бітне значення
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);   // 20-бітне значення
    int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];                                    // 16-бітне значення

    data->temperature = bme280_compensate_temperature(adc_T);   // ОБОВ'ЯЗКОВО першою -- рахує t_fine
    data->pressure    = bme280_compensate_pressure(adc_P);
    data->humidity    = bme280_compensate_humidity(adc_H);
}
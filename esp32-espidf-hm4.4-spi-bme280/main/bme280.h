#pragma once

#define PIN_SCLK        5
#define PIN_MOSI        6
#define PIN_MISO        15
#define PIN_CS          7
#define SPI_HOST_USED   SPI3_HOST      // VSPI, через GPIO matrix, до ~26 МГц

#define BME280_REG_ID   0xD0           // "адреса" з уже встановленим read-бітом (0x50 | 0x80)

void bme280_spi_bus_and_device_init(void);
void bme280_check_device(void);
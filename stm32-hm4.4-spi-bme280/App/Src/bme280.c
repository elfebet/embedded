#include "bme280.h"
#include "main.h" // extern SPI_HandleTypeDef hspi1, BME280_CS_GPIO
#include <string.h>
#include <stdio.h>

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

static bme280_calib_t calib;
static double t_fine;

static uint16_t u16_le(const uint8_t *b) {
    return (uint16_t)(b[0] | (b[1] << 8));
}

static inline void bme280_cs_low(void)  {
    HAL_GPIO_WritePin(BME280_CS_GPIO_Port, BME280_CS_Pin, GPIO_PIN_RESET);
}

static inline void bme280_cs_high(void) {
    HAL_GPIO_WritePin(BME280_CS_GPIO_Port, BME280_CS_Pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef bme280_read_regs(uint8_t reg, uint8_t *out, uint16_t len) {
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    tx[0] = reg | 0x80;
    memset(&tx[1], 0x00, len);

    bme280_cs_low();
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 1 + len, HAL_MAX_DELAY);
    bme280_cs_high();

    if (status == HAL_OK) memcpy(out, &rx[1], len);
    return status;
}

void bme280_read_calibration(void) {
    uint8_t buf_tp[26], buf_h[7];
    HAL_StatusTypeDef s1 = bme280_read_regs(0x88, buf_tp, sizeof(buf_tp));
    HAL_StatusTypeDef s2 = bme280_read_regs(0xE1, buf_h, sizeof(buf_h));
    if (s1 != HAL_OK || s2 != HAL_OK) {
        printf("Calibration read failed\r\n");
        return;
    }

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
    calib.dig_H4 = (int16_t)((buf_h[3] << 4) | (buf_h[4] & 0x0F));
    calib.dig_H5 = (int16_t)((buf_h[5] << 4) | (buf_h[4] >> 4));
    calib.dig_H6 = (int8_t)buf_h[6];
}


double bme280_compensate_temperature(int32_t adc_T) {
    double var1 = (((double)adc_T) / 16384.0 - ((double)calib.dig_T1) / 1024.0) * ((double)calib.dig_T2);
    double var2 = ((((double)adc_T) / 131072.0 - ((double)calib.dig_T1) / 8192.0) *
                   (((double)adc_T) / 131072.0 - ((double)calib.dig_T1) / 8192.0)) * ((double)calib.dig_T3);
    t_fine = var1 + var2;
    return (var1 + var2) / 5120.0;
}

double bme280_compensate_pressure(int32_t adc_P) {
    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * ((double)calib.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)calib.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)calib.dig_P4) * 65536.0);
    var1 = (((double)calib.dig_P3) * var1 * var1 / 524288.0 + ((double)calib.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)calib.dig_P1);
    if (var1 == 0.0) return 0;
    double p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)calib.dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)calib.dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)calib.dig_P7)) / 16.0;
    return p / 100.0;
}

double bme280_compensate_humidity(int32_t adc_H) {
    double var_H = (((double)t_fine) - 76800.0);
    var_H = (adc_H - (((double)calib.dig_H4) * 64.0 + ((double)calib.dig_H5) / 16384.0 * var_H)) *
            (((double)calib.dig_H2) / 65536.0 * (1.0 + ((double)calib.dig_H6) / 67108864.0 * var_H *
            (1.0 + ((double)calib.dig_H3) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((double)calib.dig_H1) * var_H / 524288.0);
    if (var_H > 100.0) var_H = 100.0;
    else if (var_H < 0.0) var_H = 0.0;
    return var_H;
}

HAL_StatusTypeDef bme280_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), value };
    bme280_cs_low();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);  // лише передача, MISO ігнорується
    bme280_cs_high();
    return status;
}

void bme280_force_measurement(void) {
    bme280_write_reg(0xF2, 0x01);   // ctrl_hum: oversampling x1
    bme280_write_reg(0xF4, 0x25);   // ctrl_meas: temp x1, press x1, forced mode
    HAL_Delay(10);
}

void bme280_read_measurements(double *temperature, double *pressure, double *humidity) {
    uint8_t raw[8];
    if (bme280_read_regs(0xF7, raw, sizeof(raw)) != HAL_OK) return;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

    *temperature = bme280_compensate_temperature(adc_T);   // першою -- рахує t_fine
    *pressure    = bme280_compensate_pressure(adc_P);
    *humidity    = bme280_compensate_humidity(adc_H);
}

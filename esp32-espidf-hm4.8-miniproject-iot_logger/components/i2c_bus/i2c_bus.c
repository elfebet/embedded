#include <stdio.h>
#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "I2C_BUS";
static i2c_master_bus_handle_t s_bus_handle;

void i2c_bus_init(int sda_pin, int scl_pin, i2c_port_t port) {
    i2c_master_bus_config_t bus_config = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = port,
        .scl_io_num                   = scl_pin,
        .sda_io_num                   = sda_pin,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &s_bus_handle));
    ESP_LOGI(TAG, "Shared I2C bus ready: SDA=GPIO%d SCL=GPIO%d", sda_pin, scl_pin);
}

i2c_master_bus_handle_t i2c_bus_handle(void) {
    return s_bus_handle;
}
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/clk_tree_defs.h"

#define I2C_PORT    I2C_NUM_0
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define I2C_FREQ_HZ 100000 // standart mode

#define SSD1306_ADDR 0x3C // 7bit, value from i2c_scan

static const char *TAG = "I2C_SCAN";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t oled_handle;

void i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C master did setup: SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1306_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &oled_handle));
}

void i2c_scan(void) {
    uint8_t start = 0x08;
    uint8_t end = 0x77;
    ESP_LOGI(TAG, "Scanning... 0x%02X..0x%02X", start, end);

    int found = 0;
    for (uint8_t i = start; i <= end; i++) {
        esp_err_t ret = i2c_master_probe(bus_handle, i, 50); // 50 ms timeout
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found device on address 0x%02X", i);
            found++;
        }
    }

    ESP_LOGI(TAG, "Total found: %d", found);
}

void ssd1306_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};   // control byte 0x00 = "command"
    ESP_ERROR_CHECK(i2c_master_transmit(
        oled_handle, 
        buf, 
        2, 
        100
    ));
}

void ssd1306_cmd2(uint8_t cmd, uint8_t arg) {
    uint8_t buf[3] = {0x00, cmd, arg};
    ESP_ERROR_CHECK(i2c_master_transmit(
        oled_handle, 
        buf, 
        3, 
        100
    ));
}

void ssd1306_clear(void) {
    // clear display buffer, set 0x00 (128x64/8 = 1024 byte)
    const uint8_t page_size = 17;
    uint8_t data[page_size];
    memset(data, 0x00, sizeof(data));
    data[0] = 0x40; // control byte = "next data for GDDRAM"

    for (int page = 0; page < 8; page++) {
        ssd1306_cmd(0xB0 + page); // select page (page addressing)
        ssd1306_cmd2(0x00, 0x10); // high/low nibble column = 0 (start column 0)
        for (int chunk = 0; chunk < 8; chunk++) {
            ESP_ERROR_CHECK(i2c_master_transmit(
                oled_handle, 
                data, 
                page_size, 
                100
            ));
        }
    }
}

void ssd1306_init_128x64(void) {
    ssd1306_cmd(0xAE);               // Display OFF
    ssd1306_cmd2(0xD5, 0x80);   // Clock divide
    ssd1306_cmd2(0xA8, 0x3F);   // Multiplex ratio = 64 row
    ssd1306_cmd2(0xD3, 0x00);   // Display offset = 0
    ssd1306_cmd(0x40);               // Start line = 0
    ssd1306_cmd2(0x8D, 0x14);   // Charge pump enable
    ssd1306_cmd2(0x20, 0x00);   // Addressing mode = horizontal
    ssd1306_cmd(0xA1);               // Segment remap
    ssd1306_cmd(0xC8);               // COM scan direction
    ssd1306_cmd2(0xDA, 0x12);   // COM pins config (128x64)
    ssd1306_cmd2(0x81, 0xCF);   // Contrast
    ssd1306_cmd2(0xD9, 0xF1);   // Pre-charge
    ssd1306_cmd2(0xDB, 0x40);   // VCOMH deselect
    ssd1306_cmd(0xA4);               // Resume to RAM content
    ssd1306_cmd(0xA6);               // Normal display (not inverted)
    ssd1306_cmd(0xAF);               // Display ON
    ESP_LOGI(TAG, "SSD1306 initialized, display turned on.");
}

// font 5x7 (each symbol — 5 colums per 8 bit)
static const uint8_t* font5x7_get(char c) {
    static const uint8_t CHAR_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00}; // ' ' (Space)
    static const uint8_t CHAR_excl[5] = {0x00, 0x00, 0x5F, 0x00, 0x00}; // '!'
    static const uint8_t CHAR_W[5] = {0x3F, 0x40, 0x38, 0x40, 0x3F}; // 'W'
    static const uint8_t CHAR_b[5] = {0x7F, 0x48, 0x44, 0x44, 0x38}; // 'b'
    static const uint8_t CHAR_c[5] = {0x38, 0x44, 0x44, 0x44, 0x20}; // 'c'
    static const uint8_t CHAR_d[5] = {0x38, 0x44, 0x44, 0x48, 0x7F}; // 'd'
    static const uint8_t CHAR_e[5] = {0x38, 0x54, 0x54, 0x54, 0x18}; // 'e'
    static const uint8_t CHAR_l[5] = {0x00, 0x41, 0x7F, 0x40, 0x00}; // 'l'
    static const uint8_t CHAR_m[5] = {0x7C, 0x04, 0x18, 0x04, 0x78}; // 'm'
    static const uint8_t CHAR_o[5] = {0x38, 0x44, 0x44, 0x44, 0x38}; // 'o'
    static const uint8_t CHAR_t[5] = {0x04, 0x3E, 0x44, 0x24, 0x00}; // 't'

    switch (c) {
        case 'W': return CHAR_W;
        case 'b': return CHAR_b;
        case 'c': return CHAR_c;
        case 'd': return CHAR_d;
        case 'e': return CHAR_e;
        case 'l': return CHAR_l;
        case 'm': return CHAR_m;
        case 'o': return CHAR_o;
        case 't': return CHAR_t;
        case '!': return CHAR_excl;
        default: return CHAR_space;
    }
}

// display row from offset (column, page)
void ssd1306_draw_string(uint8_t col, uint8_t page, const char *str) {
    ssd1306_cmd(0xB0 + page);
    ssd1306_cmd2(0x00 + (col & 0x0F), 0x10 + (col >> 4));

    while (*str) {
        const uint8_t *glyph = font5x7_get(*str);
        uint8_t buf[7];
        buf[0] = 0x40;               // control byte = "data for GDDRAM"
        memcpy(&buf[1], glyph, 5);   // 5 columns for symbol
        buf[6] = 0x00;               // 1 empty column = space between symbols
        ESP_ERROR_CHECK(i2c_master_transmit(
            oled_handle, 
            buf, 
            7, 
            100
        ));
        str++;
    }
}

void app_main(void)
{
    i2c_bus_init();
    ssd1306_init_128x64();
    ssd1306_clear();
    ssd1306_draw_string(5, 4, "Welcome to embedded!");
}
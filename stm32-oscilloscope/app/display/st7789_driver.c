#include "st7789_driver.h"
#include "st7789_defs.h"
#include "lcd_spi_port.h"
#include "gfx.h"

static st7789_spi_connected_data_t s_spi_data;
static st7789_window_data_t s_window_data;
static uint16_t s_fill_color; // It's need for async DMA

#define CS_LOW()         if (s_spi_data.cs.port) { s_spi_data.cs.port->BSRR = (uint32_t)s_spi_data.cs.pin << 16U; }
#define CS_HIGH()        if (s_spi_data.cs.port) { s_spi_data.cs.port->BSRR = s_spi_data.cs.pin; }
#define DC_COMMAND()     { s_spi_data.dc.port->BSRR = (uint32_t)s_spi_data.dc.pin << 16U; }
#define DC_DATA()        { s_spi_data.dc.port->BSRR = s_spi_data.dc.pin; }

static void st7789_reset(void) {
    if (s_spi_data.rst.port == NULL) return;

    // HAL_GPIO_WritePin(s_spi_data.rst.port, s_spi_data.rst.pin, GPIO_PIN_SET);
    // HAL_Delay(10);
    HAL_GPIO_WritePin(s_spi_data.rst.port, s_spi_data.rst.pin, GPIO_PIN_RESET);
    HAL_Delay(15);
    HAL_GPIO_WritePin(s_spi_data.rst.port, s_spi_data.rst.pin, GPIO_PIN_SET);
    HAL_Delay(20);
}

// memory data access control config
static uint8_t st7789_MADCTLConfigConvert(
    uint8_t mirror_x,
    uint8_t mirror_y,
    uint8_t exchange_xy,
    uint8_t mirror_color,
    uint8_t refresh_v,
    uint8_t refresh_h
) {
    uint8_t mem_config = 0;
    if (mirror_x)       mem_config |= ST7789_MADCTL_MX;
    if (mirror_y)       mem_config |= ST7789_MADCTL_MY;
    if (exchange_xy)    mem_config |= ST7789_MADCTL_MV;
    if (mirror_color)   mem_config |= ST7789_MADCTL_BGR;
    if (refresh_v)      mem_config |= ST7789_MADCTL_ML;
    if (refresh_h)      mem_config |= ST7789_MADCTL_MH;
    return mem_config;
}

static uint8_t st7789_MADCTLConfig(uint8_t orientation) {
    switch (orientation) {
    case GFX_ORIENTATION_PORTRAIT:
        return st7789_MADCTLConfigConvert(0, 0, 0, 0, 0, 0);
    case GFX_ORIENTATION_PORTRAIT_MIRROR:
        return st7789_MADCTLConfigConvert(1, 1, 0, 0, 1, 1);
    case GFX_ORIENTATION_LANDSCAPE:
        return st7789_MADCTLConfigConvert(1, 0, 1, 0, 0, 1);
    case GFX_ORIENTATION_LANDSCAPE_MIRROR:
        return st7789_MADCTLConfigConvert(0, 1, 1, 0, 1, 0);
    default:
        return 0; // default portrait mode
    }
}

void st7789_write_command_data(uint8_t cmd, const uint8_t *data, uint8_t count) {
    DC_COMMAND();
    CS_LOW();
    lcd_spi_transmit_blocking(&cmd, 1, LCD_SPI_SIZE_8BIT);
    if (data != NULL && count > 0) {
        DC_DATA();
        lcd_spi_transmit_blocking(data, count, LCD_SPI_SIZE_8BIT);
    }
    CS_HIGH();
}

void st7789_write_command(uint8_t cmd) {
    st7789_write_command_data(cmd, NULL, 0);
}

void st7789_init(const st7789_spi_connected_data_t *spi_data, const st7789_window_data_t *window_data) {
    s_spi_data = *spi_data;
    s_window_data = *window_data;
    lcd_spi_init(s_spi_data.spi);
    const uint8_t madctl = st7789_MADCTLConfig(s_window_data.orientation);

    CS_HIGH();

    st7789_reset();
    // st7789_write_command(ST7789_SWRESET);
    // lcd_delay(150);
    // st7789_write_command(ST7789_DISPOFF);
    // st7789_write_command(ST7789_SLPOUT);
    // lcd_delay(120);
    st7789_write_command_data(ST7789_COLMOD,    (const uint8_t[]){ ST7789_COLOR_MODE_16bit }, 1);
    st7789_write_command_data(ST7789_MADCTL,    (const uint8_t[]){ madctl }, 1);
    st7789_write_command_data(ST7789_PORCTRL,   (const uint8_t[]){ 0x0C, 0x0C, 0x00, 0x33, 0x33 }, 5); // Standard porch
//    st7789_write_command_data(ST7789_PORCTRL, (const uint8_t[]){ 0x01, 0x01, 0x00, 0x11, 0x11 }, 5); // Minimum porch (7% faster screen refresh rate)
    st7789_write_command_data(ST7789_GCTRL,     (const uint8_t[]){ 0x35 }, 1);
    st7789_write_command_data(ST7789_VCOMS,     (const uint8_t[]){ 0x19 }, 1);
    st7789_write_command_data(ST7789_LCMCTRL,   (const uint8_t[]){ 0X2C }, 1);
    st7789_write_command_data(ST7789_VDVVRHEN,  (const uint8_t[]){ 0x01 }, 1);
    st7789_write_command_data(ST7789_VRHS,      (const uint8_t[]){ 0x12 }, 1);
    st7789_write_command_data(ST7789_VDVS,      (const uint8_t[]){ 0x20 }, 1);
    st7789_write_command_data(ST7789_FRCTRL2,   (const uint8_t[]){ ST7789_FRAMERATE_60HZ }, 1);
    st7789_write_command_data(ST7789_PWCTRL1,   (const uint8_t[]){ 0xA4, 0xA1 }, 2);
    st7789_write_command_data(ST7789_PVGAMCTRL, (const uint8_t[]){ 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23 }, 14);
    st7789_write_command_data(ST7789_NVGAMCTRL, (const uint8_t[]){ 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23 }, 14);
//    st7789_write_command(ST7789_INVON);
    st7789_write_command(ST7789_SLPOUT);
    st7789_write_command(ST7789_NORON);
    st7789_write_command(ST7789_DISPON);
    lcd_delay(50);

    st7789_backlight(true);
}

void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    int16_t xs = x0 + s_window_data.x_offset;
    int16_t xe = x1 + s_window_data.x_offset;
    int16_t ys = y0 + s_window_data.y_offset;
    int16_t ye = y1 + s_window_data.y_offset;

    uint8_t args[4];
    args[0] = xs >> 8; args[1] = xs & 0xFF;
    args[2] = xe >> 8; args[3] = xe & 0xFF;
    st7789_write_command_data(ST7789_CASET, args, 4);
    args[0] = ys >> 8; args[1] = ys & 0xFF;
    args[2] = ye >> 8; args[3] = ye & 0xFF;
    st7789_write_command_data(ST7789_RASET, args, 4);
    st7789_write_command(ST7789_RAMWR);
}

void st7789_backlight(bool on) {
    if (s_spi_data.blk.port == NULL) return;
    HAL_GPIO_WritePin(s_spi_data.blk.port, s_spi_data.blk.pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void st7789_invert_colors(bool invert) {
    st7789_write_command(invert ? ST7789_INVON : ST7789_INVOFF);
}

void st7789_tear_effect(bool on) {
    st7789_write_command(on ? ST7789_TEON : ST7789_TEOFF);
}

void st7789_set_power(bool on) {
    st7789_write_command(on ? ST7789_DISPON : ST7789_DISPOFF);
}

void st7789_sleep_in(bool in) {
    st7789_write_command(in ? ST7789_SLPIN : ST7789_SLPOUT);
}

uint16_t st7789_width(void) {
    return s_window_data.width;
}

uint16_t st7789_height(void) {
    return s_window_data.height;
}

void st7789_draw_pixel(uint16_t color) {
    DC_DATA();
    CS_LOW();
    lcd_spi_transmit_blocking((const uint8_t *)&color, 1, LCD_SPI_SIZE_16BIT);
    CS_HIGH();
}

void st7789_fill_pixels(uint16_t color, uint32_t count) {
    if (count == 0) return;

    s_fill_color = color;
    DC_DATA();
    CS_LOW();
    lcd_spi_transmit_dma((const uint8_t *)&s_fill_color, count, LCD_SPI_SIZE_16BIT, LCD_SPI_MEM_DMA_FIXED);
    CS_HIGH();
}

void st7789_write_pixels(const uint16_t *pixels, uint32_t count) {
    if (pixels == NULL || count == 0) return;

    DC_DATA();
    CS_LOW();
    lcd_spi_transmit_dma((const uint8_t *)pixels, count, LCD_SPI_SIZE_16BIT, LCD_SPI_MEM_INCREMENT);
    CS_HIGH();
}

void st7789_write_pixels_blocking(const uint16_t *pixels, uint32_t count) {
    if (pixels == NULL || count == 0) return;

    DC_DATA();
    CS_LOW();
    lcd_spi_transmit_blocking((const uint8_t *)pixels, count, LCD_SPI_SIZE_16BIT);
    CS_HIGH();
}

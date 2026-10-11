#include "st7789_driver.h"
#include "st7789_defs.h"
#include "lcd_spi_port.h"
#include "gfx.h"

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
    case LCD_ORIENTATION_PORTRAIT:
        return st7789_MADCTLConfigConvert(0, 0, 0, 0, 0, 0);
    case LCD_ORIENTATION_PORTRAIT_MIRROR:
        return st7789_MADCTLConfigConvert(1, 1, 0, 0, 1, 1);
    case LCD_ORIENTATION_LANDSCAPE:
        return st7789_MADCTLConfigConvert(1, 0, 1, 0, 0, 1);
    case LCD_ORIENTATION_LANDSCAPE_MIRROR:
        return st7789_MADCTLConfigConvert(0, 1, 1, 0, 1, 0);
    default:
        return 0; // default portrait mode
    }
}

void st7789_init(uint8_t orientation) {
    lcd_port_init();
    lcd_port_reset();

    lcd_port_writeCommand(ST7789_SWRESET,   0, 0);
    lcd_port_delay(150);
    lcd_port_writeCommand(ST7789_DISPOFF,   0, 0);
    lcd_port_writeCommand(ST7789_SLPOUT,    0, 0);
    lcd_port_delay(120);
    lcd_port_writeCommand(ST7789_COLMOD,    (const uint8_t[]){ ST7789_COLOR_MODE_16bit }, 1);
    lcd_port_writeCommand(ST7789_MADCTL,    (const uint8_t[]){ st7789_MADCTLConfig(orientation) }, 1);
    lcd_port_writeCommand(ST7789_PORCTRL,   (const uint8_t[]){ 0x0C, 0x0C, 0x00, 0x33, 0x33 }, 5); // Standard porch
//    lcd_port_writeCommand(ST7789_PORCTRL, (const uint8_t[]){ 0x01, 0x01, 0x00, 0x11, 0x11 }, 5); // Minimum porch (7% faster screen refresh rate)
    lcd_port_writeCommand(ST7789_GCTRL,     (const uint8_t[]){ 0x35 }, 1);
    lcd_port_writeCommand(ST7789_VCOMS,     (const uint8_t[]){ 0x19 }, 1);
    lcd_port_writeCommand(ST7789_LCMCTRL,   (const uint8_t[]){ 0X2C }, 1);
    lcd_port_writeCommand(ST7789_VDVVRHEN,  (const uint8_t[]){ 0x01 }, 1);
    lcd_port_writeCommand(ST7789_VRHS,      (const uint8_t[]){ 0x12 }, 1);
    lcd_port_writeCommand(ST7789_VDVS,      (const uint8_t[]){ 0x20 }, 1);
    lcd_port_writeCommand(ST7789_FRCTRL2,   (const uint8_t[]){ ST7789_FRAMERATE_60HZ }, 1);
    lcd_port_writeCommand(ST7789_PWCTRL1,   (const uint8_t[]){ 0xA4, 0xA1 }, 2);
    lcd_port_writeCommand(ST7789_PVGAMCTRL, (const uint8_t[]){ 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23 }, 14);
    lcd_port_writeCommand(ST7789_NVGAMCTRL, (const uint8_t[]){ 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23 }, 14);
//    lcd_port_writeCommand(ST7789_INVON,   0, 0);
    lcd_port_writeCommand(ST7789_NORON,     0, 0);
    // --- add fill background color? ---
    lcd_port_writeCommand(ST7789_DISPON,    0, 0);
    lcd_port_delay(50);

    lcd_port_backlight(true);
}

void st7789_backlight(bool on) {
    lcd_port_backlight(on);
}

void st7789_rotation(uint8_t orientation) {
    const uint8_t args[1] = { st7789_MADCTLConfig(orientation) };
    lcd_port_writeCommand(ST7789_MADCTL, args, 1);
}

void st7789_invertColors(bool invert) {
    lcd_port_writeCommand(invert ? ST7789_INVON : ST7789_INVOFF, 0, 0);
}

void st7789_tearEffect(bool on) {
    lcd_port_writeCommand(on ? ST7789_TEON : ST7789_TEOFF, 0, 0);
}

void st7789_setPower(bool on) {
    lcd_port_writeCommand(on ? ST7789_DISPON : ST7789_DISPOFF, 0, 0);
}

void st7789_sleepIn(bool in) {
    lcd_port_writeCommand(in ? ST7789_SLPIN : ST7789_SLPOUT, 0, 0);
}

void st7789_begin(void) {
//    lcd_port_lock();
}

void st7789_end(void) {
//    lcd_port_unlock();
}

void st7789_setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t args[4];
    args[0] = x0 >> 8; args[1] = x0 & 0xFF;
    args[2] = x1 >> 8; args[3] = x1 & 0xFF;
    lcd_port_writeCommand(ST7789_CASET, args, 4);
    args[0] = y0 >> 8; args[1] = y0 & 0xFF;
    args[2] = y1 >> 8; args[3] = y1 & 0xFF;
    lcd_port_writeCommand(ST7789_RASET, args, 4);
    lcd_port_writeCommand(ST7789_RAMWR, 0, 0);
}

void st7789_writeData(const uint8_t *data, uint32_t len) {
//    lcd_port_writeData(data, len);
}

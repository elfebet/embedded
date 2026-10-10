#pragma once

// Control Registers and constant codes
#define ST7789_NOP        0x00 // no operation
#define ST7789_SWRESET    0x01 // software reset
#define ST7789_RDDID      0x04 // read display ID
#define ST7789_RDDST      0x09 // read display status
#define ST7789_SLPIN      0x10 // sleep in
#define ST7789_SLPOUT     0x11 // sleep out
#define ST7789_PTLON      0x12 // partial mode on
#define ST7789_NORON      0x13 // partial off (normal)
#define ST7789_INVOFF     0x20 // display inversion off
#define ST7789_INVON      0x21 // display inversion on
#define ST7789_DISPOFF    0x28 // display off
#define ST7789_DISPON     0x29 // display on
#define ST7789_CASET      0x2A // column address set
#define ST7789_RASET      0x2B // row address set
#define ST7789_RAMWR      0x2C // memory write
#define ST7789_RAMRD      0x2E // memory read
#define ST7789_PTLAR      0x30 // partial start/end address set
#define ST7789_TEOFF      0x34 // tearing effect line off
#define ST7789_TEON       0x35 // tearing effect line on
#define ST7789_MADCTL     0x36 // memory data access control
#define ST7789_IDMOFF     0x38 // idle mode off
#define ST7789_IDMON      0x39 // idle mode on
#define ST7789_COLMOD     0x3A // interface pixel format
#define ST7789_RAMCTRL    0xB0 // RAM control
#define ST7789_RGBCTRL    0xB1 // RGB control
#define ST7789_PORCTRL    0xB2 // porch control
#define ST7789_FRCTRL1    0xB3 // frame rate control 1
#define ST7789_GCTRL      0xB7 // gate control
#define ST7789_VCOMS      0xBB // VCOM setting
#define ST7789_LCMCTRL    0xC0 // LCM control
#define ST7789_IDSET      0xC1 // ID setting
#define ST7789_VDVVRHEN   0xC2 // VDV and VRH command enable
#define ST7789_VRHS       0xC3 // VRH set
#define ST7789_VDVS       0xC4 // VDV setting
#define ST7789_VCMOFSET   0xC5 // VCOM offset set
#define ST7789_FRCTRL2    0xC6 // FR control 2
#define ST7789_PWCTRL1    0xD0 // power control 1
#define ST7789_RDID1      0xDA // read ID 1
#define ST7789_RDID2      0xDB // read ID 2
#define ST7789_RDID3      0xDC // read ID 3
#define ST7789_RDID4      0xDD // read ID 4
#define ST7789_PVGAMCTRL  0xE0 // positive voltage gamma control
#define ST7789_NVGAMCTRL  0xE1 // negative voltage gamma control

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */
#define ST7789_MADCTL_MY  0x80 // 10000000, page address order (0 = top to bottom, 1 = bottom to top)
#define ST7789_MADCTL_MX  0x40 // 01000000, column address order (0 = left to right, 1 = right to left)
#define ST7789_MADCTL_MV  0x20 // 00100000, page/column order (0 = normal mode, 1 = reverse mode)
#define ST7789_MADCTL_ML  0x10 // 00010000, line address order (0 = LCD refresh top to bottom, 1 = bottom to top)
#define ST7789_MADCTL_BGR 0x08 // 00001000, RGB/BGR Order (0 = RGB, 1 = BGR)
#define ST7789_MADCTL_MH  0x04 // 00000100, display data latch order (0 = LCD refresh left to right, 1 = right to left)

#define ST7789_COLOR_MODE_16bit 0x55 // 101, RGB565 (16bit)
#define ST7789_COLOR_MODE_18bit 0x66 // 110, RGB666 (18bit)

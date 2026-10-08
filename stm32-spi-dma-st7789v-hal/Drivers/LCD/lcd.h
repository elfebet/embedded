#ifndef __ST7735_H__
#define __ST7735_H__

#include <stm32f4xx_hal.h>
#include <sys/_stdint.h>
#include <ugui.h>
#include <ugui_config.h>
#include "main.h"

/* For demo only. Minimum is 32, 128 and higher will enable all tests */
#define DEMO_FLASH_KB 64

/* choose a Hardware SPI port to use. */
#define LCD_HANDLE            hspi1

/* Pin connections. Use same names as in CubeMX */
#define LCD_DC                TFT_DC
#define LCD_RST               TFT_RST /* Disable if your display has no RST pin */
#define LCD_CS                TFT_CS  /* Disable if your display has no CS pin */
#define LCD_BL                TFT_BLK  /* Enable if you need backlight control */

#define USE_DMA                       /* Use DMA for transfers when possible */
//#define LCD_LOCAL_FB                /* Use local framebuffer. Needs a lot of ram, but removes flickering and redrawing glitches  */

//#define USE_ST7735                    /* LCD Selection */
#define USE_ST7789

#define LCD_ROTATION 3                /* XY rotation/mirroring. Valid values: 0...3 */

#ifdef USE_ST7735                     /* ST7735 LCD sizes */
  #define LCD_160X128
//#define LCD_128X128
//#define LCD_160X80
#elif defined USE_ST7789              /* ST7789 LCD sizes */
//#define LCD_135X240
//#define LCD_240X240
  #define LCD_240X280
#endif




#ifdef USE_ST7735
  #ifdef LCD_160X128
    #define LCD_X_SHIFT 0
    #define LCD_Y_SHIFT 0
    #if (LCD_ROTATION == 0) || (LCD_ROTATION == 2)
      #define LCD_WIDTH  128
      #define LCD_HEIGHT 160
    #elif (LCD_ROTATION == 1) || (LCD_ROTATION == 3)
      #define LCD_WIDTH  160
      #define LCD_HEIGHT 128
    #endif
  #elif defined LCD_128X128
    #define LCD_X_SHIFT 0
    #define LCD_Y_SHIFT 0
    #define LCD_WIDTH  128
    #define LCD_HEIGHT 128
  #elif defined LCD_160X80
    #define LCD_X_SHIFT 0
    #define LCD_Y_SHIFT 0
    #if (LCD_ROTATION == 0) || (LCD_ROTATION == 2)
      #define LCD_WIDTH  80
      #define LCD_HEIGHT 160
    #elif (LCD_ROTATION == 1) || (LCD_ROTATION == 3)
      #define LCD_WIDTH  160
      #define LCD_HEIGHT 80
    #endif
  #endif

  #if LCD_ROTATION == 0
    #ifdef LCD_160X80
      #define LCD_ROTATION_CMD (CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_BGR)
    #else
      #define LCD_ROTATION_CMD (CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_RGB)
    #endif
  #elif LCD_ROTATION == 1
    #ifdef LCD_160X80
      #define LCD_ROTATION_CMD (CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_BGR)
    #else
      #define LCD_ROTATION_CMD (CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_RGB)
    #endif
  #elif LCD_ROTATION == 2
    #ifdef LCD_160X80
      #define LCD_ROTATION_CMD (CMD_MADCTL_BGR)
    #else
      #define LCD_ROTATION_CMD (CMD_MADCTL_RGB)
    #endif
  #elif LCD_ROTATION == 3
    #ifdef LCD_160X80
      #define LCD_ROTATION_CMD (CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_BGR)
    #else
      #define LCD_ROTATION_CMD (CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_RGB)
    #endif
  #endif
#elif defined USE_ST7789
  #ifdef LCD_135X240
    #if (LCD_ROTATION == 0) || (LCD_ROTATION == 2)
      #define LCD_WIDTH  135
      #define LCD_HEIGHT 240
    #elif (LCD_ROTATION == 1) || (LCD_ROTATION == 3)
      #define LCD_WIDTH  240
      #define LCD_HEIGHT 135
    #endif
  #elif defined LCD_240X240
    #define LCD_WIDTH  240
    #define LCD_HEIGHT 240
    #define LCD_X_SHIFT 0
    #define LCD_Y_SHIFT 0
  #elif defined LCD_240X280
    #if (LCD_ROTATION == 0) || (LCD_ROTATION == 2)
      #define LCD_WIDTH  240
      #define LCD_HEIGHT 280
      #define LCD_X_SHIFT 0
      #define LCD_Y_SHIFT 0
    #elif (LCD_ROTATION == 1) || (LCD_ROTATION == 3)
      #define LCD_WIDTH  280
      #define LCD_HEIGHT 240
    #endif
  #endif

  #if LCD_ROTATION == 0
    #define LCD_ROTATION_CMD (CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_RGB)
    #ifdef LCD_135X240
      #define LCD_X_SHIFT 53
      #define LCD_Y_SHIFT 40
    #elif defined LCD_240X280
    #endif
  #elif LCD_ROTATION == 1
    #define LCD_ROTATION_CMD (CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_RGB)
    #ifdef LCD_135X240
      #define LCD_X_SHIFT 40
      #define LCD_Y_SHIFT 52
    #elif defined LCD_240X280
    #endif
  #elif LCD_ROTATION == 2
    #define LCD_ROTATION_CMD (CMD_MADCTL_RGB)
    #ifdef LCD_135X240
      #define LCD_X_SHIFT 52
      #define LCD_Y_SHIFT 40
    #elif defined LCD_240X280
      #define LCD_X_SHIFT 20
      #define LCD_Y_SHIFT 0
    #endif
  #elif LCD_ROTATION == 3
    #define LCD_ROTATION_CMD (CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_RGB)
    #ifdef LCD_135X240
      #define LCD_X_SHIFT 40
      #define LCD_Y_SHIFT 53
    #elif defined LCD_240X280
      #define LCD_X_SHIFT 20
      #define LCD_Y_SHIFT 0
    #endif
  #endif
#endif

/* LCD Commands */
typedef enum{
  // memory configuration parameters
  CMD_MADCTL_MY  = 0x80, // D7 bit, page address order
  CMD_MADCTL_MX  = 0x40, // D6 bit, column address order
  CMD_MADCTL_MV  = 0x20, // D5 bit, page/column order
  CMD_MADCTL_ML  = 0x10, // D4 bit, line address order
  CMD_MADCTL_RGB = 0x00, // D3 bit, RGB order?
  CMD_MADCTL_BGR = 0x08, // D3 bit, BGR order
  CMD_MADCTL_MH  = 0x04, // D2 bit, display data latch order
  // commands
  CMD_NOP        = 0x00, // no operation
  CMD_SWRESET    = 0x01, // software reset
  CMD_RDDID      = 0x04, // read display ID
  CMD_RDDST      = 0x09, // read display status
  CMD_SLPIN      = 0x10, // sleep in
  CMD_SLPOUT     = 0x11, // sleep out
  CMD_PTLON      = 0x12, // partial mode on
  CMD_NORON      = 0x13, // partial off (normal)
  CMD_INVOFF     = 0x20, // display inversion off
  CMD_INVON      = 0x21, // display inversion on
  CMD_GAMSET     = 0x26, // display inversion on
  CMD_DISPOFF    = 0x28, // display off
  CMD_DISPON     = 0x29, // display on
  CMD_CASET      = 0x2A, // column address set
  CMD_RASET      = 0x2B, // row address set
  CMD_RAMWR      = 0x2C, // memory write
  CMD_RAMRD      = 0x2E, // memory read
  CMD_PTLAR      = 0x30, // partial start/end address set
  CMD_MADCTL     = 0x36, // memory data access control
  CMD_IDMOFF     = 0x38, // idle mode off
  CMD_IDMON      = 0x39, // idle mode on
  CMD_COLMOD     = 0x3A, // interface pixel format
  CMD_RAMCTRL    = 0xB0, // RAM control
  CMD_RGBCTRL    = 0xB1, // RGB control
  CMD_FRMCTR1    = 0xB1, // TODO: REMOVE IT
  CMD_FRMCTR2    = 0xB2, // TODO: REMOVE IT
  CMD_FRMCTR3    = 0xB3, // TODO: REMOVE IT
  CMD_PORCTRL    = 0xB2, // porch control
  CMD_FRCTRL1    = 0xB3, // frame rate control 1
  CMD_INVCTR     = 0xB4, // TODO: REMOVE IT
  CMD_PARCTRL    = 0xB5, // TODO: REMOVE IT
  CMD_DISSET5    = 0xB6, // TODO: REMOVE IT
  CMD_GCTRL      = 0xB7, // gate control
  CMD_VCOMS      = 0xBB, // VCOM setting
  CMD_LCMCTRL    = 0xC0, // LCM control
  CMD_PWCTR1     = 0xC0, // TODO: REMOVE IT
  CMD_PWCTR2     = 0xC1, // TODO: REMOVE IT
  CMD_PWCTR3     = 0xC2, // TODO: REMOVE IT
  CMD_PWCTR4     = 0xC3, // TODO: REMOVE IT
  CMD_PWCTR5     = 0xC4, // TODO: REMOVE IT
  CMD_PWCTR6     = 0xFC, // TODO: REMOVE IT
  CMD_IDSET      = 0xC1, // ID setting
  CMD_VDVVRHEN   = 0xC2, // VDV and VRH command enable
  CMD_VRHS       = 0xC3, // VRH set
  CMD_VDVS       = 0xC4, // VDV setting
  CMD_VMCTR1     = 0xC5, // TODO: REMOVE IT
  CMD_VCMOFSET   = 0xC5, // VCOM offset set
  CMD_FRCTRL2    = 0xC6, // FR control 2
  CMD_PWCTRL1    = 0xD0, // power control 1
  CMD_RDID1      = 0xDA, // read ID 1
  CMD_RDID2      = 0xDB, // read ID 2
  CMD_RDID3      = 0xDC, // read ID 3
  CMD_RDID4      = 0xDD, // TODO: REMOVE IT
  CMD_GMCTRP1    = 0xE0, // PVGAMCTRL (Positive Voltage Gamma Control)
  CMD_GMCTRN1    = 0xE1, // NVGAMCTRL (Negative Voltage Gamma Control)
  CMD_COLOR_MODE_12bit = 0x33, // `011` 12bit/pixel
  CMD_COLOR_MODE_16bit = 0x55, // `101` 16bit/pixel
  CMD_COLOR_MODE_18bit = 0x66, // `110` 18bit/pixel
}lcd_cmds;

#define color565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))
#define ABS(x) ((x) > 0 ? (x) : -(x))

#define LCD_CON(a,b)  a##b
#define LCD_PIN(pin, out)   ( LCD_CON(pin,_GPIO_Port->BSRR) = (out ? LCD_CON(pin,_Pin) : LCD_CON(pin,_Pin)<<16 ))

extern SPI_HandleTypeDef    LCD_HANDLE;

void LCD_init(void);
void LCD_SetRotation(uint8_t m);
void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color);
void LCD_DrawPixelFB(int16_t x, int16_t y, uint16_t color);
int8_t LCD_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);

/* Graphical functions. */
int8_t LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawImage(uint16_t x, uint16_t y, UG_BMP* bmp);
void LCD_InvertColors(uint8_t invert);

/* Text functions. */
void LCD_PutChar(uint16_t x, uint16_t y, char ch, UG_FONT* font, uint16_t color, uint16_t bgcolor);
void LCD_PutStr(uint16_t x, uint16_t y,  char *str, UG_FONT* font, uint16_t color, uint16_t bgcolor);

/* Extended Graphical functions. */
/* Command functions */
void LCD_TearEffect(uint8_t tear);

/* Simple test function. */
void LCD_Test(void);

#endif // __ST7735_H__

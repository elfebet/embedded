#pragma once
#include <stdbool.h>
#include "main.h"

#define LCD_SPI_PORT hspi1
extern SPI_HandleTypeDef LCD_SPI_PORT;

#define LCD_USE_DMA 0
#define LCD_DMA_MIN_SIZE  16

#define LCD_RST_PORT TFT_RST_GPIO_Port
#define LCD_RST_PIN  TFT_RST_Pin
#define LCD_DC_PORT  TFT_DC_GPIO_Port
#define LCD_DC_PIN   TFT_DC_Pin
// comment below if unused
#define LCD_CS_PORT  TFT_CS_GPIO_Port
#define LCD_CS_PIN   TFT_CS_Pin
// comment below if unused
#define LCD_BLK_PORT TFT_BLK_GPIO_Port
#define LCD_BLK_PIN  TFT_BLK_Pin

typedef enum {
    LCD_TYPE_PIXEL,            // Малювання одного пікселя (8 біт, без DMA)
    LCD_TYPE_FILL,             // Заливка кольором/прямокутник (16 біт, DMA без інкременту)
    LCD_TYPE_IMAGE,            // Масив/Картинка з пам'яті (16 біт, DMA з інкрементом)
    LCD_TYPE_FONT,             // Шрифт/Текст (зазвичай 8 або 16 біт залежно від вашого рендерера)
    LCD_TYPE_FRAMEBUFFER_8BIT,
    LCD_TYPE_FRAMEBUFFER_16BIT,
} LCD_DataType;

void lcd_port_init(void);
void lcd_port_reset(void);
void lcd_port_backlight(bool on);
void lcd_port_delay(uint32_t ms);

void lcd_port_lock(void);
void lcd_port_unlock(void);

void lcd_port_writeCommand(uint8_t cmd, const uint8_t *args, uint8_t arg_count);
void lcd_port_writeData(const uint8_t *buff, size_t buff_size, LCD_DataType dataType);


//// перемикати потрібно ТІЛЬКИ ПІСЛЯ переводу DC і СS
//// 1. перемикнули DC + CS
//// 2. виставили режим
//// 3. передали данні
//// 4. підняли СS
//// 5. повернули в 8 бітний режим, default mode
//// default mode should be: mem_increase, mode_8bit
//void lcd_port_memMode(uint8_t memInc, uint8_t modeSz);

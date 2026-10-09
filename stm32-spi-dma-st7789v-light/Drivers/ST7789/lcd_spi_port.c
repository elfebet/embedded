#include "lcd_spi_port.h"

#ifdef LCD_CS_PIN
  #define CS_LOW()   HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET)
  #define CS_HIGH()  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET)
#else
  #define CS_LOW()
  #define CS_HIGH()
#endif
#define DC_LOW()   HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_RESET)
#define DC_HIGH()  HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_SET)

// SET:   LCD_CS_PORT->BSRR = LCD_CS_PIN;
// RESET: LCD_CS_PORT->BSRR = (uint32_t)LCD_CS_PIN << 16U;

typedef enum {
    LCD_DMA_MEM_UNKNOWN = -1,
    LCD_DMA_MEM_FIXED = 0, // fill window / rect / line
    LCD_DMA_MEM_INCREMENT = 1  // draw image, frame buffer, font?
} LCD_DMA_MemMode;

typedef enum {
    LCD_SPI_SIZE_UNKNOWN = -1,
    LCD_SPI_8BIT  = 0, // commands, draw 1 pixel, 8-bit frame buffer
    LCD_SPI_16BIT = 1, // image, font?, fill rect, fill straight lines, 16-bit frame buffer
} LCD_SPI_Size;

typedef struct {
    LCD_SPI_Size spi_sz;
    LCD_SPI_Size dma_sz;
    LCD_DMA_MemMode dma_mem_mode;
} spi_config_t;

// LCD_Fill: fixed, 16bit, else: mode_16bit
// LCD_FillArea: fixed, 16bit, else: mode_16bit
// LCD_DrawImage: increase, 16bit, else: mode_16bit

//#define mem_fixed     0
//#define mem_increase  1
//#define mode_8bit     0 // для команд, координати вікна, 1 піксель
//#define mode_16bit    1 // для малювання, масив пікселів, заливка прямокутника


static spi_config_t spi_config = {
  .spi_sz = LCD_SPI_SIZE_UNKNOWN,
  .dma_sz = LCD_SPI_SIZE_UNKNOWN,
  .dma_mem_mode = LCD_DMA_MEM_UNKNOWN,
};

// Sets SPI interface word size
// call this function ONLY before transit
static void setSPI_Size(LCD_SPI_Size size) {
    if (spi_config.spi_sz == size) return;

    __HAL_SPI_DISABLE(&LCD_SPI_PORT);
    spi_config.spi_sz = size;
    if (size == LCD_SPI_16BIT) {
        LCD_SPI_PORT.Init.DataSize = SPI_DATASIZE_16BIT;
        LCD_SPI_PORT.Instance->CR1 |= SPI_CR1_DFF;
    } else if (size == LCD_SPI_8BIT) {
        LCD_SPI_PORT.Init.DataSize = SPI_DATASIZE_8BIT;
        LCD_SPI_PORT.Instance->CR1 &= ~(SPI_CR1_DFF);
    } else {
        // assert?
    }
    // __HAL_SPI_ENABLE will calls inside HAL_SPI_Transmit
}

/**
 * @brief Configures DMA / SPI interface.
 * @param memInc Enable/disable memory address increase
 * @param mode16 Enable/disable 16 bit mode (disabled = 8 bit)
 * @return none
 */
#ifdef LCD_USE_DMA
// call this function ONLY before transit DMA
static void setDMAMemMode(LCD_DMA_MemMode mode, LCD_SPI_Size size) {
    setSPI_Size(size);
    if (spi_config.dma_sz == size && spi_config.dma_mem_mode == mode) return;

    spi_config.dma_sz = size;
    spi_config.dma_mem_mode = mode;
    __HAL_DMA_DISABLE(LCD_SPI_PORT.hdmatx);

#ifdef DMA_SxCR_EN
    while ((LCD_SPI_PORT.hdmatx->Instance->CR & DMA_SxCR_EN) != RESET);
#elif defined DMA_CCR_EN
    while ((LCD_SPI_PORT.hdmatx->Instance->CCR & DMA_CCR_EN) != RESET);
#endif

    if (mode == LCD_DMA_MEM_INCREMENT) {
        LCD_SPI_PORT.hdmatx->Init.MemInc = DMA_MINC_ENABLE;
#ifdef DMA_SxCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CR |= DMA_SxCR_MINC;
#elif defined DMA_CCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CCR |= DMA_CCR_MINC;
#endif
    } else if (mode == LCD_DMA_MEM_FIXED) {
        LCD_SPI_PORT.hdmatx->Init.MemInc = DMA_MINC_DISABLE;
#ifdef DMA_SxCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CR &= ~(DMA_SxCR_MINC);
#elif defined DMA_CCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CCR &= ~(DMA_CCR_MINC);
#endif
    } else {
        // assert?
    }

    if (size == LCD_SPI_16BIT) {
        LCD_SPI_PORT.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        LCD_SPI_PORT.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
#ifdef DMA_SxCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CR = (LCD_SPI_PORT.hdmatx->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk)) |
                                            (1<<DMA_SxCR_PSIZE_Pos | 1<<DMA_SxCR_MSIZE_Pos);
#elif defined DMA_CCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CCR = (LCD_SPI_PORT.hdmatx->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk)) |
                                             (1<<DMA_CCR_PSIZE_Pos | 1<<DMA_CCR_MSIZE_Pos);
#endif
    } else if (size == LCD_SPI_8BIT) {
        LCD_SPI_PORT.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        LCD_SPI_PORT.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
#ifdef DMA_SxCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CR = (LCD_SPI_PORT.hdmatx->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk));
#elif defined DMA_CCR_EN
        LCD_SPI_PORT.hdmatx->Instance->CCR = (LCD_SPI_PORT.hdmatx->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk));
#endif
    } else {
        // assert?
    }

    // __HAL_SPI_ENABLE will calls inside HAL_SPI_Transmit_DMA
}

spi_config_t make_spiConfig(LCD_DMA_MemMode mode, LCD_SPI_Size size) {
    return (spi_config_t){
        .spi_sz = size,
        .dma_sz = size,
        .dma_mem_mode = mode,
   };
}

spi_config_t dataType_to_spiConfig(LCD_DataType dataType) {
    switch (dataType) {
    case LCD_TYPE_PIXEL:
        return make_spiConfig(LCD_DMA_MEM_INCREMENT, LCD_SPI_8BIT);
    case LCD_TYPE_FILL:
        return make_spiConfig(LCD_DMA_MEM_FIXED, LCD_SPI_16BIT);
    case LCD_TYPE_IMAGE:
        return make_spiConfig(LCD_DMA_MEM_INCREMENT, LCD_SPI_16BIT);
    case LCD_TYPE_FONT:
        return make_spiConfig(LCD_DMA_MEM_INCREMENT, LCD_SPI_8BIT); // check it!!
    case LCD_TYPE_FRAMEBUFFER_8BIT:
        return make_spiConfig(LCD_DMA_MEM_INCREMENT, LCD_SPI_8BIT);
    case LCD_TYPE_FRAMEBUFFER_16BIT:
        return make_spiConfig(LCD_DMA_MEM_INCREMENT, LCD_SPI_16BIT);
    default:
        return make_spiConfig(LCD_DMA_MEM_INCREMENT, LCD_SPI_8BIT);
    }
}

#endif

//LCD_Fill: fixed, 16bit, else: mode_16bit
//LCD_FillArea: fixed, 16bit, else: mode_16bit
//LCD_DrawImage: increase, 16bit, else: mode_16bit


//void lcd_port_memMode(uint8_t memInc, uint8_t modeSz) {
//#ifdef LCD_USE_DMA
//    setDMAMemMode(memInc, modeSz);
//#else
//    setSPI_Size(modeSz);
//#endif
//}

void lcd_port_init(void) {
    CS_HIGH();
    DC_HIGH();
}

void lcd_port_reset(void) {
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);
}

void lcd_port_backlight(bool on) {
#ifdef LCD_BLK_PIN
    HAL_GPIO_WritePin(LCD_BLK_PORT, LCD_BLK_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void lcd_port_delay(uint32_t ms) {
    HAL_Delay(ms);
}

void lcd_port_lock(void) {
    CS_LOW();
}

void lcd_port_unlock(void) {
    CS_HIGH();
}

void lcd_port_writeCommand(uint8_t cmd, const uint8_t *args, uint8_t arg_count) {
    DC_LOW();
    CS_LOW();
    setSPI_Size(LCD_SPI_8BIT);
    HAL_SPI_Transmit(&LCD_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
    if (args && arg_count) {
        DC_HIGH();
        HAL_SPI_Transmit(&LCD_SPI_PORT, args, arg_count, HAL_MAX_DELAY);
    }
    CS_HIGH();
}

void lcd_port_writeData(const uint8_t *buff, size_t buff_size, LCD_DataType dataType) {
    DC_LOW();
    CS_LOW();

#ifdef LCD_USE_DMA
    spi_config_t config = dataType_to_spiConfig(dataType);
    setDMAMemMode(config.dma_mem_mode, config.dma_sz);
#endif

    while (buff_size > 0) {
        uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
#ifdef LCD_USE_DMA
        if (buff_size > LCD_DMA_MIN_SIZE) {
            HAL_SPI_Transmit_DMA(&LCD_SPI_PORT, buff, chunk_size);
            while (HAL_DMA_GetState(LCD_SPI_PORT.hdmatx) != HAL_DMA_STATE_READY);
            if (spi_config.dma_mem_mode == LCD_DMA_MEM_INCREMENT) {
                // WHY WE DO IT???
                buff += (spi_config.dma_sz == LCD_SPI_16BIT) ? chunk_size : chunk_size*2;
            }
        } else {
            HAL_SPI_Transmit(&LCD_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
            // WHY WE DO IT???
            buff += (spi_config.spi_sz == LCD_SPI_16BIT) ? chunk_size : chunk_size*2;
        }
#else
        HAL_SPI_Transmit(&LCD_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        // SHOULD WE ADD THIS BELOW???
//        buff += (spi_config.spi_sz == LCD_SPI_16BIT) ? chunk_size : chunk_size*2;
#endif
        buff_size -= chunk_size;
    }

    CS_HIGH();
}

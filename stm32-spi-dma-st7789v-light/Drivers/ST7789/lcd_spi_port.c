#include "lcd_spi_port.h"

static SPI_HandleTypeDef *s_hspi;
static int8_t s_spi_size = -1;
static int8_t s_dma_size = -1;
static int8_t s_dma_mem = -1;

void lcd_spi_init(SPI_HandleTypeDef *hspi) {
    s_hspi = hspi;
    s_spi_size = -1;
    s_dma_size = -1;
    s_dma_mem = -1;
}

void lcd_spi_set_size(lcd_spi_size_t size) {
    if (s_hspi == NULL || s_spi_size == size) return;

    s_spi_size = size;
    __HAL_SPI_DISABLE(s_hspi);

// #define SET_BIT(REG, BIT)     ((REG) |= (BIT))
// #define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))


    if (size == LCD_SPI_SIZE_16BIT) {
        s_hspi->Init.DataSize = SPI_DATASIZE_16BIT;
        s_hspi->Instance->CR1 |= SPI_CR1_DFF;
    } else { // size == LCD_SPI_SIZE_8BIT
        s_hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        s_hspi->Instance->CR1 &= ~(SPI_CR1_DFF);
    }
    // __HAL_SPI_ENABLE will calls inside HAL_SPI_Transmit
}

void lcd_spi_set_dma(lcd_spi_mem_t mem, lcd_spi_size_t size) {
    lcd_spi_set_size(size);

    if (s_hspi == NULL || s_hspi->hdmatx == NULL) return;
    if (s_dma_size == size && s_dma_mem == mem) return;

    s_dma_size = size;
    s_dma_mem = mem;

    DMA_HandleTypeDef *hdma = s_hspi->hdmatx;
    __HAL_DMA_DISABLE(hdma);

#ifdef DMA_SxCR_EN
    while ((hdma->Instance->CR & DMA_SxCR_EN) != RESET);
#elif defined DMA_CCR_EN
    while ((hdma->Instance->CCR & DMA_CCR_EN) != RESET);
#endif

    if (mem == LCD_SPI_MEM_FIXED) {
        hdma->Init.MemInc = DMA_MINC_DISABLE;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR &= ~(DMA_SxCR_MINC);
#elif defined DMA_CCR_EN
        hdma->Instance->CCR &= ~(DMA_CCR_MINC);
#endif
    } else { // (mem == LCD_SPI_MEM_INCREMENT)
        hdma->Init.MemInc = DMA_MINC_ENABLE;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR |= DMA_SxCR_MINC;
#elif defined DMA_CCR_EN
        hdma->Instance->CCR |= DMA_CCR_MINC;
#endif
    }

    if (size == LCD_SPI_SIZE_16BIT) {
        hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR = (hdma->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk)) | (1u << DMA_SxCR_PSIZE_Pos | 1u << DMA_SxCR_MSIZE_Pos);
#elif defined DMA_CCR_EN
        hdma->Instance->CCR = (hdma->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk)) | (1u << DMA_CCR_PSIZE_Pos | 1u << DMA_CCR_MSIZE_Pos);
#endif
    } else { // (size == LCD_SPI_SIZE_8BIT)
        hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR = (hdma->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk));
#elif defined DMA_CCR_EN
        hdma->Instance->CCR = (hdma->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk));
#endif
    }

    // __HAL_SPI_ENABLE will calls inside HAL_SPI_Transmit_DMA
}

void lcd_delay(uint32_t ms) {
     HAL_Delay(ms);
}

uint8_t lcd_spi_dma_ready() {
    if (s_hspi == NULL || s_hspi->hdmatx == NULL) return 0;
    return HAL_DMA_GetState(s_hspi->hdmatx) == HAL_DMA_STATE_READY;
}

void lcd_spi_dma_wait(void) {
    if (s_hspi == NULL || s_hspi->hdmatx == NULL) return;
    while (HAL_DMA_GetState(s_hspi->hdmatx) != HAL_DMA_STATE_READY);
}

static void transmit(const uint8_t *data, uint32_t count, uint8_t use_dma) {
    if (s_hspi == NULL || data == NULL) return;

    const uint8_t unit_bytes = (s_spi_size == LCD_SPI_SIZE_16BIT) ? 2u : 1u;
    const int mem_fixed = s_dma_mem == LCD_SPI_MEM_FIXED;

    while (count > 0u) {
        uint16_t chunk = (count > 65535u) ? 65535u : (uint16_t)count;
#if ST7789_USE_DMA
        if (use_dma && s_hspi->hdmatx != NULL) {
            HAL_SPI_Transmit_DMA(s_hspi, data, chunk);
            lcd_spi_dma_wait();
        } else
#endif
        {
            if (mem_fixed) {
                for (uint32_t i = 0; i < chunk; i++) {
                    HAL_SPI_Transmit(s_hspi, data, 1, HAL_MAX_DELAY);
                }
            } else {
                HAL_SPI_Transmit(s_hspi, data, chunk, HAL_MAX_DELAY);
            }
        }

        if (!mem_fixed) {
            data += (uint32_t)chunk * unit_bytes;
        }
        count -= chunk;
    }
}

void lcd_spi_transmit_blocking(const uint8_t *data, uint32_t count)  {
    transmit(data, count, 0);
}
void lcd_spi_transmit_dma(const uint8_t *data, uint32_t count) {
    transmit(data, count, 1);
}

/*
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
*/

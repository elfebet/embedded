#include "lcd_spi_port.h"
#include "stm32f4xx.h"

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
    if (size == LCD_SPI_SIZE_16BIT) {
        s_hspi->Init.DataSize = SPI_DATASIZE_16BIT;
        SET_BIT(s_hspi->Instance->CR1, SPI_CR1_DFF);
    } else { // size == LCD_SPI_SIZE_8BIT
        s_hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        CLEAR_BIT(s_hspi->Instance->CR1, SPI_CR1_DFF);
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

    if (mem == LCD_SPI_MEM_DMA_FIXED) {
        hdma->Init.MemInc = DMA_MINC_DISABLE;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR &= ~(DMA_SxCR_MINC);
        // CLEAR_BIT(hdma->Instance->CR, DMA_SxCR_MINC);
#elif defined DMA_CCR_EN
        hdma->Instance->CCR &= ~(DMA_CCR_MINC);
        // CLEAR_BIT(hdma->Instance->CCR, DMA_CCR_MINC);
#endif
    } else { // (mem == LCD_SPI_MEM_INCREMENT)
        hdma->Init.MemInc = DMA_MINC_ENABLE;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR |= DMA_SxCR_MINC;
        // SET_BIT(hdma->Instance->CR, DMA_SxCR_MINC);
#elif defined DMA_CCR_EN
        hdma->Instance->CCR |= DMA_CCR_MINC;
        // SET_BIT(hdma->Instance->CCR, DMA_CCR_MINC);
#endif
    }

    if (size == LCD_SPI_SIZE_16BIT) {
        hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR = (hdma->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk)) | (1u << DMA_SxCR_PSIZE_Pos | 1u << DMA_SxCR_MSIZE_Pos);
        // MODIFY_REG(hdma->Instance->CR, DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk, (1U << DMA_SxCR_PSIZE_Pos) | (1U << DMA_SxCR_MSIZE_Pos));
#elif defined DMA_CCR_EN
        hdma->Instance->CCR = (hdma->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk)) | (1u << DMA_CCR_PSIZE_Pos | 1u << DMA_CCR_MSIZE_Pos);
        // MODIFY_REG(hdma->Instance->CCR, DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk, (1U << DMA_CCR_PSIZE_Pos) | (1U << DMA_CCR_MSIZE_Pos));
#endif
    } else { // (size == LCD_SPI_SIZE_8BIT)
        hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
#ifdef DMA_SxCR_EN
        hdma->Instance->CR = (hdma->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk));
        // CLEAR_BIT(hdma->Instance->CR, DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk);
#elif defined DMA_CCR_EN
        hdma->Instance->CCR = (hdma->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk));
        // CLEAR_BIT(hdma->Instance->CCR, DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk);
#endif
    }
    // __HAL_DMA_ENABLE will calls inside HAL_SPI_Transmit_DMA
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
    // TODO: ТРЕБА ПЕРЕРОБИТИ НА ОЧІКУВАННЯ З ТАЙМАУТОМ
    while (HAL_DMA_GetState(s_hspi->hdmatx) != HAL_DMA_STATE_READY);
}

static void transmit(const uint8_t *data, uint32_t count, uint8_t use_dma) {
    if (s_hspi == NULL || data == NULL) return;

    const uint8_t dma = use_dma && s_hspi->hdmatx != NULL;
    const uint8_t mem_fixed = dma && s_dma_mem == LCD_SPI_MEM_DMA_FIXED;
    const uint8_t unit_bytes = s_spi_size == LCD_SPI_SIZE_16BIT ? 2u : 1u;

    while (count > 0u) {
        uint16_t chunk = (count > 65535u) ? 65535u : (uint16_t)count;
        if (dma) {
            HAL_SPI_Transmit_DMA(s_hspi, data, chunk);
            lcd_spi_dma_wait();
        } else {
            // TODO: ТРЕБА ПЕРЕРОБИТИ НА ОЧІКУВАННЯ З ТАЙМАУТОМ. HAL_MAX_DELAY зло!
            HAL_SPI_Transmit(s_hspi, data, chunk, HAL_MAX_DELAY);
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

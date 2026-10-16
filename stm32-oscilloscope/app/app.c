#include "app.h"
#include "main.h"
#include "config.h"
#include "st7789_driver.h"
#include "gfx.h"
#include "gfx_text.h"

void display_init(void) {
    st7789_spi_connected_data_t spi_data = {
      .spi = &hspi1,
      .dc = {TFT_DC_GPIO_Port, TFT_DC_Pin},
      .cs = {TFT_CS_GPIO_Port, TFT_CS_Pin},
      .rst = {TFT_RST_GPIO_Port, TFT_RST_Pin},
      .blk = {TFT_BLK_GPIO_Port, TFT_BLK_Pin},
  };
  st7789_window_data_t window_data = {
      .width = 240,
      .height = 320,
      .orientation = GFX_ORIENTATION_PORTRAIT,
  };

  st7789_init(&spi_data, &window_data);
}

void app_start(void) {
    display_init();
}

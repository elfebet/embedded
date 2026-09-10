idf_component_register(
    SRCS "u8g2_hal.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_i2c
        u8g2
)
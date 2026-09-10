idf_component_register(
    SRCS "env_spi.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_spi
        bme280
        esp_timer
)
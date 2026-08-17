idf_component_register(
    SRCS "rate_dial.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_gpio
        esp_driver_pcnt
    PRIV_REQUIRES config
)
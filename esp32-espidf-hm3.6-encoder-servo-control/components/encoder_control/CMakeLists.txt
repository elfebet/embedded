idf_component_register(
    SRCS "encoder_control.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_driver_gpio
        esp_timer
    PRIV_REQUIRES
        esp_driver_pcnt
)



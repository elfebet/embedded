idf_component_register(
    SRCS "drop_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_adc
        esp_timer
    PRIV_REQUIRES config
)
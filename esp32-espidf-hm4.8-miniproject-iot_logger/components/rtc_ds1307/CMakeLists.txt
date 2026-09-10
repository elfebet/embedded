idf_component_register(
    SRCS "rtc_ds1307.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_i2c
        i2c_bus
)


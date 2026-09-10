idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_i2c
        i2c_bus
        rtc_ds1307
        bme280
        esp_timer
        u8g2_hal
        u8g2
)
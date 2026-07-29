idf_component_register(
            SRCS "load_driver.c"
            INCLUDE_DIRS "include"
            REQUIRES
                driver
                esp_driver_ledc
            PRIV_REQUIRES esp_driver_gpio
)


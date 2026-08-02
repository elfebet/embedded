idf_component_register(
            SRCS "ntc.c"
            INCLUDE_DIRS "include"
            REQUIRES 
                driver
            PRIV_REQUIRES
                config
                esp_adc
                esp_timer
                esp_driver_gpio
)


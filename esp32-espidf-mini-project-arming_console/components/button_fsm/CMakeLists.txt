idf_component_register(
            SRCS "button_fsm.c"
            INCLUDE_DIRS "include"
            REQUIRES 
                driver
            PRIV_REQUIRES esp_driver_gpio
)


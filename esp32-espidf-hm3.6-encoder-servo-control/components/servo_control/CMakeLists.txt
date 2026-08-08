idf_component_register(
    SRCS "servo_control.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_driver_gpio
        esp_driver_ledc
        esp_timer
)
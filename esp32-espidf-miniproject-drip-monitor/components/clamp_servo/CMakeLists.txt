idf_component_register(
    SRCS "clamp_servo.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc
    PRIV_REQUIRES config
)


# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c         # list the source files of this component
    INCLUDE_DIRS    "." # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
        esp_driver_i2c
        esp_driver_spi
        nvs_flash
        spi_flash
        i2c_bus
        env_i2c
        env_spi
        rtc_ds1307
        oled_display
        ctrl_adc
        led_ctrl
        wifi_prov
        mqtt_link
        cjson
    PRIV_REQUIRES       # optional, list the private requirements
)

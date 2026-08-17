# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c         # list the source files of this component
    INCLUDE_DIRS   "."  # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
        drop_sensor
        temp_sensor
        rate_dial
        clamp_servo
        alarm_driver
    PRIV_REQUIRES       # optional, list the private requirements
)

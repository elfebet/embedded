# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Exclude the Arduino-specific entry point for ESP-IDF builds.
list(FILTER app_sources EXCLUDE REGEX "/main_arduino\\.cpp|/main_stm32_arduino\\.cpp$")

idf_component_register(SRCS ${app_sources})

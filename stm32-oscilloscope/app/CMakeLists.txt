file(GLOB APP_SOURCES CONFIGURE_DEPENDS "*.c" "src/*.c" "display/*.c" "input/*.c")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${APP_SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/display
    ${CMAKE_CURRENT_SOURCE_DIR}/input
)

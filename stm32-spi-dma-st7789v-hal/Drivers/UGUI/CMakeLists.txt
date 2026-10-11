# Автоматично шукаємо всі .c файли в UGUI та підпапках
file(GLOB_RECURSE UGUI_SOURCES "*.c")

# 2. Виключаємо непотрібні файли зі списку (ДОДАТИ ЦЕЙ РЯДОК)
list(REMOVE_ITEM UGUI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/ugui_sim_x11.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ugui_sim.c"
)

target_sources(STM32_Drivers PRIVATE ${UGUI_SOURCES})
target_include_directories(stm32cubemx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
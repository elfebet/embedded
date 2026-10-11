# Автоматично шукаємо абсолютно всі .c файли всередині цієї папки
file(GLOB LCD_SOURCES "*.c" "src/*.c")

# Додаємо їх до загальної бібліотеки мікроконтролера
target_sources(STM32_Drivers PRIVATE ${LCD_SOURCES})

# Автоматично підключаємо цю папку для інклудів .h
target_include_directories(stm32cubemx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
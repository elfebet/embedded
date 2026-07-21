# Homework for lesson 2.5 (Timers)

Project works for 3 platforms
1. Espressif IDE platform realization - `main_espidf.c`
2. ESP32 arduino - `main_arduino.cpp`
3. STM32 arduino - `main_stm32_arduino.cpp`

## Circuit

```
ESP32-S3
────────┐             ┌─────────────────────┐
        |             |                     |
GPIO5   |-------------| IN                  |
GND     |-------------| GND                 |
3.3V    |-------------| VCC                 |
────────┘             |     Relay Module    |
                      |                     |
                      | COM    NO      NC   |
                      └──┬─────┬───────┬────┘
                         |     |
                         |     |
                         |     |
                         |     |
                         | ┌───┴────────────────┐         GND
                         | | MOTOR with FAN     |-----------------|
                         | └───┬────────────────┘                 |
                         |     │                                  |
                         |     |          LED Indicator     GND   |
                         |     +-----[R220Om]-----|>|-------------|
                         |                                        |
+5.0V (External) --------|                                        |
 GND  (External) -------------------------------------------------|
```
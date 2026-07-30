## Модуль 2.1. Обмеження ресурсів та особливості

- кнопка яка змінює стан світлодіоду (off/on/blinking)
- використовуємо `constexpr`, `volatile`
- використовуємо бітові маски (бітові операції) для налаштування LED

```
constexpr uint32_t LED_MASK = 1u << LED_PIN;
GPIO.enable |= LED_MASK; // підключаємо пін, аналог pinMode(LED_PIN, OUTPUT)
GPIO.out &= ~LED_MASK; // вимикаємо діод
GPIO.out |= LED_MASK; // вмикаємо діод
GPIO.out ^= LED_MASK; // змінюємо стан діоду на протилежний
```

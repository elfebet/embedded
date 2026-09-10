# 🌡️ 🔗 📡 ПРОЄКТ 9 · IOT · ~8.5 ГОД · ESP-IDF v5.5.4

# Логер даних та керування: I²C/SPI/DMA-телеметрія з дисплеєм і публікацією по MQTT

> Покрокова практика: на ESP32-S3 (**чистий ESP-IDF, без Arduino**) будуємо
> навчальну станцію логування й керування — той самий клас логіки, що в
> реальних промислових IoT-вузлах збирає показання з кількох шин одночасно й
> віддає їх у хмару по мережі. Один і той самий фізичний параметр
> (температура/тиск/вологість) читається **одночасно двома способами** з
> двох окремих датчиків BME280 — одного на **I²C** (Модулі 4.2-4.3:
> адресація, регістри, WTR) і одного на **SPI** (Модулі 4.4-4.5: CPOL/CPHA,
> Chip ID, цілочисельна компенсація Bosch) — щоб наочно порівняти дві шини
> на тому самому обладнанні, а не лише в теорії. На тій самій I²C-шині
> живуть ще два пристрої: **годинник реального часу Tiny RTC (DS1307)**,
> що тримає час навіть між перезавантаженнями завдяки власній батарейці, і
> **OLED-дисплей SSD1306**, що на старті 5 секунд показує знайому анімовану
> мордочку вовка (той самий бітмап і той самий принцип моргання, що й у
> прикладі `u8g2_clock_and_wolf_logo_EXAMPLE.md` з Модуля 4.3), а потім
> перемикається на живий дашборд: час і показання обох датчиків просто на
> платі, без потреби відкривати консоль. Ручка потенціометра задає поріг
> тривоги, зчитуваний АЦП у **continuous-DMA режимі** (Модуль 4.6) — без
> жодного циклу опитування CPU. Wi-Fi-мережу пристрій отримує не з
> хардкодженого рядка в коді, а через **SoftAP-провіжинінг**: ESP32-S3
> сам піднімає тимчасову точку доступу, телефон підключається до неї й
> передає реальні SSID/пароль через застосунок **ESP SoftAP Provisioning**.
> Усі показання пакуються в JSON і йдуть через **MQTT** (pub/sub — нова
> тема, що розбирається тут із нуля) на **власний MQTT-брокер** (Mosquitto),
> піднятий у тій самій локальній мережі — без хмари й без TLS-складності.
> Той самий MQTT-клієнт одночасно **слухає** тему керування й дистанційно
> перемикає **LED-індикатор тривоги** — "Керування" з назви проєкту працює
> і локально (ручка), і по мережі (MQTT), керуючи тим самим індикатором.
> Завершує практику живий дебаг зібраної системи через вбудований
> **USB-Serial-JTAG** (Модуль 4.7).

**🧩 ESP-IDF з нуля · 🏗️ Модульні компоненти · 🔗 I²C проти SPI на тому самому датчику · 🐺 Вовк-заставка + дисплей-дашборд · 🎚️ ADC-DMA без CPU · 📲 Wi-Fi provisioning (SoftAP) · 📶 MQTT pub/sub на власному брокері · 🐞 Live-debug JTAG · 🏆 Челенджі**

*Інтегрує Модулі 4.2-4.3 (I²C: адресація, регістри, WTR, NACK/ACK, RTC/OLED через u8g2), 4.4-4.5 (SPI: CPOL/CPHA, BME280, компенсація), 4.6 (DMA: ADC continuous), 4.7 (JTAG/SWD/ICP, live-debug). Wi-Fi provisioning та MQTT (pub/sub, легковаговість) розбираються в Кроці 8 як нова тема.*

---

## 🎯 МЕТА ПРОЄКТУ

**У кінці ви отримаєте:**

1. Модульний ESP-IDF-проєкт (`components/…`): `i2c_bus` (спільна шина),
   `env_i2c` (4.2-4.3), `env_spi` (4.4-4.5), `bme280_compensate` (спільна
   математика компенсації), `rtc_ds1307` (годинник реального часу),
   `u8g2_hal` (HAL-міст `u8g2` ↔ `driver/i2c_master.h`, Модуль 4.2,
   ЕРАТА п.1), `oled_display` (вовк-заставка + дашборд), `ctrl_adc` (4.6),
   `led_ctrl`, `wifi_prov` (SoftAP-провіжинінг), `mqtt_link` (esp-mqtt) —
   кожен відповідає за одну periferію/протокол і нічого не знає про
   "логер" чи "MQTT-брокер" в цілому.
2. Наочне, апаратне порівняння I²C проти SPI на **тому самому фізичному
   параметрі**: два окремі BME280 читають те саме приміщення двома різними
   протоколами одночасно.
3. Три пристрої на одній спільній I²C-шині (BME280 `0x76`, DS1307 `0x68`,
   SSD1306 `0x3C`) — та сама конфігурація "OLED+RTC" з Модуля 4.2-4.3,
   тепер працює всередині більшої системи, а не як окрема вправа.
4. Дисплей, що на старті 5 секунд показує знайому анімацію вовка (Модуль
   4.3), а потім автоматично перемикається на живий дашборд: час із
   апаратного RTC і показання обох датчиків одночасно — без консолі.
5. Контрольну ручку порогу тривоги на ADC1 у `continuous`+DMA режимі
   (Модуль 4.6) — CPU жодного разу не опитує лінію АЦП напряму.
6. Wi-Fi без жодного хардкодженого пароля: пристрій сам піднімає SoftAP,
   телефон передає реальні дані мережі через офіційний застосунок
   Espressif — і робочий MQTT pub/sub з нуля на власному брокері
   (Mosquitto), з коректним системним часом через SNTP, який до того ж
   одноразово "засіює" апаратний RTC — і живий дебаг через вбудований
   USB-Serial-JTAG (Модуль 4.7) на вже зібраній, багатозадачній системі.

---

## ⚠️ ЕРАТА · виправлення під реальну збірку (ESP-IDF v5.5.4, N16R8)

> Нижче — п'ять розбіжностей між текстом практики й тим, що реально
> вимагає компілятор/лінкер на конкретному тулчейні (GCC 14.2.0,
> ESP-IDF v5.5.4, плата ESP32-S3-DevKitC-1 **N16R8**, 16 МБ flash + 8 МБ
> Octal PSRAM). Усі п'ять уже виправлені й у коді нижче за текстом, і в
> зібраному репозиторії — цей блок лише пояснює, **чому** код виглядає
> саме так у Кроках 1, 3, 7, 8, 9, щоб не довелось відкривати цю ж
> помилку самостійно.

**1. `u8g2_hal` — окремий компонент, а не файли в `main/`.** Крок 3,
Частина 2, дослівно каже "продовжує `setup_u8g2.sh`... з Модуля 4.2" —
а той скрипт кладе `u8g2_hal.c/.h` **у `main/`** як звичайні
source-файли компонента `main`. Але `components/oled_display/CMakeLists.txt`
(нижче, Крок 3) уже містить `REQUIRES ... u8g2_hal`, ніби це окремий
компонент. Це внутрішня суперечність тексту: CMake шукає компонент
`u8g2_hal` і не знаходить його ("unknown name"), бо файли фізично
лежать не там. **Виправлення:** створити `components/u8g2_hal/` як
повноцінний компонент (`include/u8g2_hal.h` + `u8g2_hal.c` +
`CMakeLists.txt` з `REQUIRES u8g2 driver esp_rom log freertos`) і
перенести туди вміст `u8g2_hal.c/.h` дослівно з Модуля 4.2 — жодних
змін у самому коді HAL-моста не потрібно, лише в тому, **де** він
живе.

**2. `adc_continuous_register_event_callbacks()` у ESP-IDF v5.5.4 приймає
третій аргумент.** Крок 7, `ctrl_adc.c` — сигнатура функції в
поточному ESP-IDF: `(handle, const adc_continuous_evt_cbs_t *cbs, void
*user_data)`. Виклик з лише двома аргументами не компілюється ("too
few arguments"). **Виправлення:** `adc_continuous_register_event_callbacks(adc_handle,
&cbs, NULL)` — `user_data` тут не потрібен, callback і так бере
значення з `static` змінних файлу.

**3. Заголовок FreeRTOS — `event_groups.h` (множина), а не
`event_group.h`.** Крок 8, Частина 0, `wifi_prov.c` — у поточному
ESP-IDF файл називається `freertos/event_groups.h`; однини версії не
існує, компіляція падає з `fatal error: ... No such file or
directory`. **Виправлення:** `#include "freertos/event_groups.h"`.

**4. `-Werror=format-truncation` на буферах часу.** Крок 3
(`oled_display.c`, `line_time`) і Крок 9 (`app_main.c`, `utc_time`) —
поля `rtc_time_t.hour/min/sec` мають тип `uint8_t` (діапазон 0-255 з
погляду компілятора, хоч логічно й 0-59/0-23), тож GCC 14 вважає, що
`"%02u:%02u:%02u"` теоретично може вивести 3-значне число й переповнити
буфер `[10]`/`[9]`, і зі стандартним для цього проєкту `-Werror=all`
це — помилка збірки, не попередження. **Виправлення:** запас у
буфері — `char line_time[16]` (Крок 3) і `char utc_time[16]` (Крок 9);
на реальний вивід (завжди 8 символів `HH:MM:SS`) це не впливає.

**5. Розмір flash і таблиця розділів — під N16R8 (16 МБ), а не
дефолтні 2 МБ.** Крок 1 не згадує про це явно, а стандартний
`idf.py create-project` + `set-target esp32s3` лишає в `sdkconfig`
`Flash Size = 2MB` і таблицю розділів `Single factory app` (factory-
розділ лише 1 МБ). З усіма компонентами проєкту (u8g2, wifi_provisioning
+ protocomm + mbedTLS, esp-mqtt, lwip) готовий бінарник — **≈1.06 МБ**,
і він banально не влазить у 1-мегабайтний розділ ("app partition is too
small", overflow ≈40 КБ). **Виправлення:** одразу після `idf.py set-target
esp32s3` (Крок 1) виконати
```bash
idf.py menuconfig
# Serial flasher config -> Flash size -> 16 MB
# Partition Table -> Partition Table -> Single factory app (large, no OTA)
```
— на платі N16R8 це не марнотратство: 16 МБ flash дозволяють мати
2-мегабайтний factory-розділ і ще залишок під NVS/OTA в майбутньому.

**6. Усі рядки `ESP_LOGx(...)` — англійською, не українською.**
У ранніх чернетках цієї практики текст логів був українською (для
наочності самого посібника). На реальному залізі це проблема, а не
косметика: `idf.py monitor`/стандартний послідовний термінал Windows
типово працює в кодуванні, яке не відображає кирилічний UTF-8 з ESP32
коректно — замість тексту в консолі видно суцільні `?????????????????`,
і незрозуміло, яке саме повідомлення щойно вивелося (це особливо
заважає під час діагностики збоїв, коли рядок логу — єдина зачіпка).
**Виправлення:** усі рядки всередині `ESP_LOGI/W/E(...)` у коді нижче —
англійською; україномовними лишаються тільки коментарі (`//`, `/* */`)
— вони не потрапляють у скомпільований бінарник і не йдуть у
консоль, тож кодування термінала на них не впливає.

**7. `wifi_prov_init_and_connect()` могла зависнути навічно з мовчазним
NVS.** Крок 8, Частина 0 — оригінальний варіант чекав IP
(`xEventGroupWaitBits(..., portMAX_DELAY)`) без жодного таймауту. Якщо
плата вже провіжинена (є збережені SSID/пароль у NVS), а ці дані
застаріли або невірні — `esp_wifi_connect()` мовчки провалюється знову
й знову (обробник `WIFI_EVENT_STA_DISCONNECTED` перевикликає
`esp_wifi_connect()` без жодного логу), очікування триває вічно, у
консолі повна тиша, а SoftAP `PROV_iot_logger_XXXX` для повторного
налаштування більше **ніколи не з'являється сама** — єдиний вихід був
руками стерти розділ NVS через `esptool erase_region`. **Виправлення**
(детально в Кроці 8) — таймаут `STA_CONNECT_TIMEOUT_MS` (5 хв): якщо
IP не отримано вчасно, плата сама скидає провіжинінг
(`wifi_prov_mgr_reset_provisioning()`) і перезавантажується в SoftAP.
Спершу таймаут був 20 с, але цього виявилось замало: на практиці
роутер/Wi-Fi іноді відповідає повільніше, особливо одразу після повного
вимкнення й увімкнення живлення плати (а не простого reset), і 20-секундний
таймаут спрацьовував хибно — стирав повністю робочі облікові дані.

---

## 01 · Крок 0 · Обладнання, схема, безпека ⏱ 35 хв

**Що на столі:** ESP32-S3-DevKitC-1 N16R8 (USB-C, кабель **даних**), дві
макетні плати, набір перемичок, два модулі BME280 (один буде підключений у
I²C-режимі, другий — у SPI-режимі, **перевірте, що на другому модулі
фізично виведений `CSB`**, інакше SPI-режим недоступний — той самий
застережний момент, що й у Модулі 4.4), модуль годинника реального часу
**Tiny RTC I2C (DS1307)** з батарейкою **CR2032**, OLED-дисплей **SSD1306
128×64 (I²C)**, потенціометр **B10K**, звичайний **LED 5 мм (червоний)**
як індикатор тривоги, резистор **330 Ом**, за потреби **2× pull-up
4.7 кОм** (лише якщо жоден з трьох I²C-модулів не має власних on-board
pull-up — перевірте мультиметром опір `SDA`/`SCL` до `3V3` на зібраній
шині), логічний аналізатор **24 МГц / 8 каналів**.

**Що на комп'ютері/телефоні/в мережі:** комп'ютер (чи Raspberry Pi) у тій
самій Wi-Fi мережі з установленим **MQTT-брокером Mosquitto** (Крок 8,
Частина 1) — жодного хмарного акаунту не потрібно, і мобільний телефон із
застосунком **ESP SoftAP Provisioning** (Android Google Play чи iOS App
Store, офіційний застосунок Espressif) — ним передаються реальні дані
вашого Wi-Fi на пристрій без жодного хардкоду в коді (Крок 8, Частина 0).

### Список компонентів і посилання на покупку

| Компонент | Магазин | Посилання |
|---|---|---|
| ESP32-S3-DevKitC-1 N16R8 | Arduino.ua | https://arduino.ua/ru/prod7600-plata-razrabotchika-esp32-s3-n16r8-type-c |
| Макетна плата (830 точок), 2 шт | ArduShop | https://ardushop.in.ua/arduino/prototyping-board-without-soldering-mb-102-830-points |
| Набір проводів-перемичок (M-M, M-F) | Mini-Tech | https://www.mini-tech.com.ua/ua/provoda-dlya-maketirovaniya-papa-mama |
| BME280 (I²C/SPI, 6-pin) — 2 шт | Arduino.ua | https://arduino.ua/prod2830-datchik-temperaturi-i-vlagnosti-bme280 |
| Tiny RTC I2C модуль (DS1307 + CR2032) | Arduino.ua | https://arduino.ua/prod671-modyl-chasov-realnogo-vremeni-tiny-rtc-i2c |
| OLED-дисплей SSD1306 128×64 (I²C) | Arduino.ua | https://arduino.ua/prod2402-oled-displei-0-96-128x64-i2c-bilii |
| Потенціометр B10K (лінійний, 3-pin) | Arduino.ua | https://arduino.ua/prod4420-lineinii-potenciometr-10kom |
| Світлодіод 5 мм (червоний) | Arduino.ua | https://arduino.ua/prod372-Svetodiod_krasnii |
| Набір резисторів (330 Ом, 4.7 кОм і ін.) | Geekmatic | https://geekmatic.in.ua/ua/resistor_kit_30_values |
| Логічний аналізатор 24 МГц/8 каналів | Rozetka | https://rozetka.com.ua/ua/282178298/p282178298/ |

> **🟠 МІНІ-ЗАВДАННЯ**
> Ціни й наявність змінюються — перш ніж замовляти, звірте актуальні дані
> прямо на сторінці товару, особливо для BME280 (переконайтесь, що модуль
> дійсно 6-піновий з виведеним `CSB`/`SDO`) і для Tiny RTC (переконайтесь,
> що батарейка CR2032 в комплекті або вже вставлена — без неї модуль
> губить час при кожному відключенні живлення, як і звичайний DS1307).

```
ПІНАУТ ПРОЄКТУ (ESP32-S3 N16R8, безпечні GPIO — уникаємо 0/3 (strapping),
26-32 (Quad flash/PSRAM), 33-37 (Octal PSRAM, специфічно для N16R8),
39-46 (JTAG/USB-Serial-JTAG/strapping). SPI-піни нижче — 5/6/16/17, а не
18/19/23, як в оригінальному варіанті цієї практики: той самий фізичний
принцип, просто інший вільний набір GPIO.)

  GPIO4  ── вхід ADC1_CH3       ← потенціометр B10K — поріг тривоги, DMA-continuous (Модуль 4.6)
  GPIO5  ── вихід SPI SCLK      → BME280 №2 (SPI)
  GPIO6  ── вихід SPI SDI(MOSI) → BME280 №2 (SPI)
  GPIO8  ── I2C SDA (спільна)   ↔ BME280 №1 (0x76), Tiny RTC DS1307 (0x68), SSD1306 (0x3C)
  GPIO9  ── I2C SCL (спільна)   ↔ той самий трипристрійний шлейф, що й GPIO8
  GPIO15 ── вихід, логіка       → LED-індикатор тривоги (через резистор 330 Ом на GND)
  GPIO16 ── вхід SPI SDO(MISO)  ← BME280 №2 (SPI)
  GPIO17 ── вихід SPI CS        → BME280 №2 (SPI), CSB

Три I2C-пристрої на GPIO8/9 -- та сама конфігурація "OLED+RTC" з Модуля 4.2
(там ще була EEPROM AT24C32E, тут її немає -- лише два адресні сусіди
BME280-I2C). Кожен адресується окремо (0x76/0x68/0x3C), лінія фізично одна.

LED керується напряму з GPIO -- індикатор споживає лише кілька мА, тож ані
реле, ані окреме джерело живлення тут не потрібні (на відміну від реального
силового навантаження на кшталт вентилятора чи нагрівача).
```

### Схема складання на макетних платах ⏱ додатково 15 хв

```
                 МАКЕТНА ПЛАТА 1 — I2C-шина (3 пристрої) + ADC + LED
                 ЛІВА СИЛОВА РЕЙКА (+ червона = 3.3V, − синя = GND)
   +  ─────────────────────────────────────────────────────────────────
   −  ─────────────────────────────────────────────────────────────────
        a  b  c  d  e     f  g  h  i  j
   1     .  .  .  .  .     .  .  .  .  .   ← BME280 №1 (I2C): VCC/GND → рейки,
   2     .  .  .  .  .     .  .  .  .  .     SCL→GPIO9, SDA→GPIO8, SDO→рейка(−) [адр. 0x76]
   3     .  .  .  .  .     .  .  .  .  .
   4     .  .  .  .  .     .  .  .  .  .   ← Tiny RTC DS1307: VCC/GND → рейки, SCL→GPIO9, SDA→GPIO8
   5     .  .  .  .  .     .  .  .  .  .
   6     .  .  .  .  .     .  .  .  .  .   ← SSD1306 OLED: VCC/GND → рейки, SCL→GPIO9, SDA→GPIO8
   7     .  .  .  .  .     .  .  .  .  .
   8     .  .  .  .  .     .  .  .  .  .   ← потенціометр B10K: крайні ніжки → рейки, повзунок → GPIO4
   9     .  .  .  .  .     .  .  .  .  .
  10     .  .  .  .  .     .  .  .  .  .   ← LED: анод → f10 → резистор 330 Ом → g10 → GPIO15,
  11     .  .  .  .  .     .  .  .  .  .     катод → рейка (−)
        a  b  c  d  e     f  g  h  i  j
   +  ─────────────────────────────────────────────────────────────────
   −  ─────────────────────────────────────────────────────────────────

                 МАКЕТНА ПЛАТА 2 — SPI-шина
   +  ─────────────────────────────────────────────────────────────────
   −  ─────────────────────────────────────────────────────────────────
        a  b  c  d  e     f  g  h  i  j
   1     .  .  .  .  .     .  .  .  .  .   ← BME280 №2 (SPI): VCC/GND → рейки,
   2     .  .  .  .  .     .  .  .  .  .     SCK→GPIO5, SDI→GPIO6, SDO→GPIO16, CSB→GPIO17
        a  b  c  d  e     f  g  h  i  j
   +  ─────────────────────────────────────────────────────────────────
   −  ─────────────────────────────────────────────────────────────────

  Обидві плати діляться лише спільним GND з ESP32-S3 — I2C і SPI фізично
  незалежні одна від одної (Модуль 4.4, Частина 5, Завдання 5.2).
```

**Порядок складання.**
1. `GND` ESP32-S3 → синя (−) рейка обох плат (спільна); `3V3` → червона (+)
   обох плат.
2. Мультиметром перевірте відсутність КЗ між рейками на обох платах.
3. BME280 №1 у I²C-режимі: `SDO`→GND (адреса `0x76`), `SCL`→GPIO9,
   `SDA`→GPIO8.
4. Tiny RTC DS1307 і SSD1306 — на ту саму пару `GPIO8`/`GPIO9`, паралельно
   до BME280 №1 (три пристрої, одна фізична шина). Вставте батарейку CR2032
   в Tiny RTC, якщо ще не вставлена.
5. Мультиметром (опір) перевірте `SDA`/`SCL` відносно `3V3` — якщо жоден з
   трьох модулів не тягне лінію вгору сам, додайте 2× 4.7 кОм pull-up
   (Модуль 4.2).
6. Потенціометр — за схемою плати 1.
7. LED: анод через резистор 330 Ом до GPIO15, катод до GND (той самий
   патерн, що й у попередніх проєктах — Модулі 1-3).
8. BME280 №2 у SPI-режимі: усі чотири лінії `SCK`/`SDI`/`SDO`/`CSB` до
   окремих GPIO (не залишайте `CSB` підтягнутим до VCC).
9. Ще раз прозвоніть обидві плати на КЗ — і лише тоді підключайте USB.

> **🔴 ТЕХНІКА БЕЗПЕКИ**
> Перемикання — лише при вийнятому USB. BME280 і SSD1306 — 3.3В-пристрої,
> не подавайте 5В (Tiny RTC-модулі зазвичай мають власний стабілізатор і
> толерують 5В на VCC — перевірте маркування вашого конкретного модуля).

**ПЛАН ПРАКТИКИ**

| Час | Крок | Що робимо |
|---|---|---|
| 0:00–0:35 | Крок 0 | Схема, безпека |
| 0:35–1:05 | Крок 1 | Ініціалізація проєкту, модульна архітектура |
| 1:05–1:50 | Крок 2 | I²C: адресація, WTR, читання BME280 (Модулі 4.2-4.3) |
| 1:50–2:40 | Крок 3 | Tiny RTC + OLED: вовк-заставка й дашборд (Модулі 4.2-4.3) |
| 2:40–3:05 | Крок 4 | I²C: обробка помилок NACK (Модуль 4.3) |
| 3:05–3:45 | Крок 5 | SPI: Mode 0, Chip ID, калібрування (Модуль 4.4) |
| 3:45–4:20 | Крок 6 | SPI: цілочисельна компенсація T→P→H (Модуль 4.5) |
| 4:20–5:00 | Крок 7 | DMA: ADC continuous — ручка порогу (Модуль 4.6) |
| 5:00–5:55 | Крок 8 | Wi-Fi provisioning (SoftAP) + MQTT на власному брокері |
| 5:55–6:25 | Крок 9 | Інтеграція: головний цикл логера й LED |
| 6:25–6:50 | Крок 10 | Живий дебаг через USB-Serial-JTAG (Модуль 4.7) |
| 6:50–7:05 | Крок 11 | Просте тестування |
| 7:05–8:35 | Челенджі | 6 завдань на розширення |

---

## 02 · Крок 1 · Ініціалізація проєкту та модульна архітектура ⏱ 30 хв

> **🟢 МЕТА**
> Створити самостійний ESP-IDF-проєкт `iot_logger` з модульним розбиттям
> по periferії/протоколу — той самий принцип, що в Проєктах 6-8.

```bash
idf.py create-project iot_logger
cd iot_logger
idf.py set-target esp32s3
mkdir -p components/bme280_compensate/include components/i2c_bus/include ^
         components/env_i2c/include components/env_spi/include ^
         components/rtc_ds1307/include components/oled_display/include ^
         components/ctrl_adc/include components/led_ctrl/include ^
         components/wifi_prov/include components/mqtt_link/include ^
         components/u8g2_hal/include
```

> **🔴 ЕРАТА (п. 5)** — плата з Кроку 0 (**N16R8**) має 16 МБ flash, а не
> дефолтні для щойно створеного проєкту 2 МБ. Одразу тут, до першої ж
> збірки, виконайте `idf.py menuconfig` → `Serial flasher config` →
> `Flash size` → `16 MB`, і `Partition Table` → `Partition Table` →
> `Single factory app (large, no OTA)`. Якщо пропустити цей крок, лінкер
> впаде лише в самому кінці Кроку 9 ("app partition is too small") —
> набагато дорожче діагностувати постфактум, ніж виправити зараз.

```
iot_logger/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild              ← Крок 8: CONFIG_MQTT_BROKER_URI, CONFIG_MQTT_CLIENT_IDENTIFIER
│   └── app_main.c                    ← Крок 9: головний цикл логера
└── components/
    ├── bme280_compensate/             ← спільна математика Bosch, без I/O
    ├── i2c_bus/                       ← Крок 2: ОДНА спільна I2C-шина на трьох споживачів
    │   ├── CMakeLists.txt
    │   ├── include/i2c_bus.h
    │   └── i2c_bus.c
    ├── env_i2c/                       ← Кроки 2, 4 (Модулі 4.2-4.3) -- BME280 на i2c_bus
    ├── rtc_ds1307/                    ← Крок 3 (Модуль 4.2-4.3) -- годинник на i2c_bus
    │   ├── CMakeLists.txt
    │   ├── include/rtc_ds1307.h
    │   └── rtc_ds1307.c
    ├── oled_display/                  ← Крок 3 (Модуль 4.3) -- вовк + дашборд, теж на i2c_bus
    │   ├── CMakeLists.txt
    │   ├── include/oled_display.h
    │   └── oled_display.c
    ├── env_spi/                       ← Кроки 5-6 (Модулі 4.4-4.5)
    ├── ctrl_adc/                      ← Крок 7 (Модуль 4.6)
    ├── led_ctrl/                      ← Крок 9 -- LED-індикатор тривоги
    │   ├── CMakeLists.txt
    │   ├── include/led_ctrl.h
    │   └── led_ctrl.c
    ├── wifi_prov/                     ← Крок 8, Частина 0 -- SoftAP-провіжинінг
    │   ├── CMakeLists.txt
    │   ├── include/wifi_prov.h
    │   └── wifi_prov.c
    └── mqtt_link/                     ← Крок 8, Частина 2 -- esp-mqtt клієнт власного брокера
        ├── CMakeLists.txt
        ├── include/mqtt_link.h
        └── mqtt_link.c
```

**`main/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES env_i2c env_spi rtc_ds1307 oled_display ctrl_adc led_ctrl wifi_prov mqtt_link
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`i2c_bus` — окремий, новий компонент саме тому, що на шині тепер
>   троє споживачів** — у наївнішій версії проєкту `env_i2c` сам викликав
>   `i2c_new_master_bus()`. З появою `rtc_ds1307` і `oled_display` на тій
>   самій парі `GPIO8`/`GPIO9` виклик `i2c_new_master_bus()` з трьох різних
>   файлів на той самий фізичний порт (`I2C_NUM_0`) означав би трикратну
>   спробу зайняти один і той самий апаратний ресурс — ESP-IDF поверне
>   помилку на другому й третьому виклику. Рішення — той самий принцип,
>   що й у Модулі 4.2 (один `i2c_bus_init()` на всі три пристрої шини),
>   лише тепер явно винесений в окремий компонент.
> - **`led_ctrl`, а не `relay_ctrl`** — індикатор тривоги тут звичайний
>   LED, керований напряму з GPIO через резистор; жодного силового
>   навантаження, жодної гальванічної розв'язки, жодного окремого джерела
>   живлення не потрібно — компонент навмисно названий за тим, чим він
>   насправді керує.

> **🟠 МІНІ-ЗАВДАННЯ**
> Якби замість LED потрібно було керувати реальним силовим навантаженням
> (вентилятором, нагрівачем), які саме зміни знадобились би в `led_ctrl`
> (перейменування на `relay_ctrl`, реле-модуль між GPIO15 і навантаженням),
> а які компоненти проєкту лишились би геть незмінними? Це і є пряма
> ілюстрація того, чому "бізнес-логіка" (Крок 9) не залежить від
> конкретної фізичної природи виконавчого механізму.

---

## 03 · Крок 2 · I²C: адресація, WTR, читання BME280 ⏱ 45 хв

> **🟢 МЕТА**
> Створити спільну I²C-шину (Крок 1) і прочитати BME280 №1, застосувавши
> напряму Write-Then-Read і burst-read з Модулів 4.2-4.3 — цього разу на
> реальному сенсорі, а не на RTC/EEPROM.

**`components/i2c_bus/include/i2c_bus.h`:**
```c
#pragma once
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

void i2c_bus_init(void);                        // викликати РІВНО один раз, до будь-якого з трьох *_init()
i2c_master_bus_handle_t i2c_bus_handle(void);    // повертає готовий дескриптор шини

#ifdef __cplusplus
}
#endif
```

**`components/i2c_bus/i2c_bus.c`:**
```c
#include "i2c_bus.h"
#include "esp_log.h"

#define I2C_PORT       I2C_NUM_0
#define I2C_SDA_PIN    8
#define I2C_SCL_PIN    9

static const char *TAG = "I2C_BUS";
static i2c_master_bus_handle_t s_bus_handle;

void i2c_bus_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_PORT,
        .scl_io_num                   = I2C_SCL_PIN,
        .sda_io_num                   = I2C_SDA_PIN,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = false,   // зовнішні 4.7 кОм, як у Модулі 4.2 -- перевірте Крок 0
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &s_bus_handle));
    ESP_LOGI(TAG, "Shared I2C bus ready: SDA=GPIO%d SCL=GPIO%d (3 devices)", I2C_SDA_PIN, I2C_SCL_PIN);
}

i2c_master_bus_handle_t i2c_bus_handle(void)
{
    return s_bus_handle;
}
```

**`components/i2c_bus/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "i2c_bus.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
```

**`components/bme280_compensate/include/bme280_compensate.h`:**
```c
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Кожен ФІЗИЧНИЙ екземпляр BME280 має власні заводські коефіцієнти і
// власний t_fine -- на відміну від Модулів 4.4-4.5, де був один global
// static t_fine (там працював лише один датчик за раз). Тут два датчики
// одночасно, тож поле винесене в структуру -- інакше t_fine SPI-датчика
// переписав би t_fine I2C-датчика й обидві компенсації тихо зламались би.
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
    int32_t  t_fine;
} bme280_calib_t;

void bme280_parse_calib(bme280_calib_t *c, const uint8_t buf_tp[26], const uint8_t buf_h[7]);
int32_t  bme280_compensate_T(bme280_calib_t *c, int32_t adc_T);   // 0.01 град.Ц (5123 -> 51.23)
uint32_t bme280_compensate_P(bme280_calib_t *c, int32_t adc_P);   // Q24.8, Па -- поділити на 256.0
uint32_t bme280_compensate_H(bme280_calib_t *c, int32_t adc_H);   // Q22.10, %RH -- поділити на 1024.0

#ifdef __cplusplus
}
#endif
```

**`components/bme280_compensate/bme280_compensate.c`** (офіційні цілочисельні
формули Bosch, Модуль 4.5, Частина 3 — дослівно ті самі, лише `t_fine`
тепер поле `c->t_fine`, а не глобальна `static`):
```c
#include "bme280_compensate.h"

static uint16_t u16_le(const uint8_t *b) { return (uint16_t)(b[0] | (b[1] << 8)); }

void bme280_parse_calib(bme280_calib_t *c, const uint8_t buf_tp[26], const uint8_t buf_h[7])
{
    c->dig_T1 = u16_le(&buf_tp[0]);
    c->dig_T2 = (int16_t)u16_le(&buf_tp[2]);
    c->dig_T3 = (int16_t)u16_le(&buf_tp[4]);
    c->dig_P1 = u16_le(&buf_tp[6]);
    c->dig_P2 = (int16_t)u16_le(&buf_tp[8]);
    c->dig_P3 = (int16_t)u16_le(&buf_tp[10]);
    c->dig_P4 = (int16_t)u16_le(&buf_tp[12]);
    c->dig_P5 = (int16_t)u16_le(&buf_tp[14]);
    c->dig_P6 = (int16_t)u16_le(&buf_tp[16]);
    c->dig_P7 = (int16_t)u16_le(&buf_tp[18]);
    c->dig_P8 = (int16_t)u16_le(&buf_tp[20]);
    c->dig_P9 = (int16_t)u16_le(&buf_tp[22]);
    c->dig_H1 = buf_tp[25];

    c->dig_H2 = (int16_t)u16_le(&buf_h[0]);
    c->dig_H3 = buf_h[2];
    c->dig_H4 = (int16_t)((buf_h[3] << 4) | (buf_h[4] & 0x0F));
    c->dig_H5 = (int16_t)((buf_h[5] << 4) | (buf_h[4] >> 4));
    c->dig_H6 = (int8_t)buf_h[6];
}

int32_t bme280_compensate_T(bme280_calib_t *c, int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)c->dig_T1 << 1))) * ((int32_t)c->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)c->dig_T1)) * ((adc_T >> 4) - ((int32_t)c->dig_T1))) >> 12) *
            ((int32_t)c->dig_T3)) >> 14;
    c->t_fine = var1 + var2;
    T = (c->t_fine * 5 + 128) >> 8;
    return T;
}

uint32_t bme280_compensate_P(bme280_calib_t *c, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)c->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)c->dig_P6;
    var2 = var2 + ((var1 * (int64_t)c->dig_P5) << 17);
    var2 = var2 + (((int64_t)c->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)c->dig_P3) >> 8) + ((var1 * (int64_t)c->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)c->dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)c->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)c->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)c->dig_P7) << 4);
    return (uint32_t)p;
}

uint32_t bme280_compensate_H(bme280_calib_t *c, int32_t adc_H)
{
    int32_t v_x1_u32r;
    v_x1_u32r = c->t_fine - (int32_t)76800;
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)c->dig_H4) << 20) - (((int32_t)c->dig_H5) * v_x1_u32r)) +
                 (int32_t)16384) >> 15) *
                 (((((((v_x1_u32r * (int32_t)c->dig_H6) >> 10) *
                 (((v_x1_u32r * (int32_t)c->dig_H3) >> 11) + (int32_t)32768)) >> 10) + (int32_t)2097152) *
                 (int32_t)c->dig_H2 + 8192) >> 14));
    v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * (int32_t)c->dig_H1) >> 4);
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    return (uint32_t)(v_x1_u32r >> 12);
}
```

**`components/bme280_compensate/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "bme280_compensate.c"
    INCLUDE_DIRS "include"
)
```

**`components/env_i2c/include/env_i2c.h`:**
```c
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void env_i2c_init(void);
bool env_i2c_read(float *temp_C, float *press_hPa, float *hum_RH);   // false = помилка I2C (Крок 4)

#ifdef __cplusplus
}
#endif
```

**`components/env_i2c/env_i2c.c`:**
```c
#include "env_i2c.h"
#include "bme280_compensate.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define BME280_ADDR    0x76     // SDO -> GND (Модуль 4.2, Додаток Б: BME280 = 0x76/0x77)
#define I2C_FREQ_HZ    100000

#define REG_CTRL_HUM   0xF2
#define REG_CTRL_MEAS  0xF4
#define REG_PRESS_MSB  0xF7

static const char *TAG = "ENV_I2C";
static i2c_master_dev_handle_t bme_handle;
static bme280_calib_t calib;

// WTR "у чистому вигляді" -- Модуль 4.3: один виклик, довільна довжина
static esp_err_t wtr(uint8_t reg, uint8_t *out, size_t len)
{
    return i2c_master_transmit_receive(bme_handle, &reg, 1, out, len, 100);
}

void env_i2c_init(void)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BME280_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle(), &dev_cfg, &bme_handle));

    uint8_t chip_id = 0;
    ESP_ERROR_CHECK(wtr(0xD0, &chip_id, 1));
    ESP_LOGI(TAG, "I2C BME280 id=0x%02X (expected 0x60)", chip_id);

    uint8_t buf_tp[26], buf_h[7];
    ESP_ERROR_CHECK(wtr(0x88, buf_tp, sizeof(buf_tp)));
    ESP_ERROR_CHECK(wtr(0xE1, buf_h, sizeof(buf_h)));
    bme280_parse_calib(&calib, buf_tp, buf_h);

    ESP_LOGI(TAG, "env_i2c ready, address 0x%02X on the shared bus", BME280_ADDR);
}

bool env_i2c_read(float *temp_C, float *press_hPa, float *hum_RH)
{
    uint8_t ctrl_hum = 0x01;                                  // osrs_h = x1
    if (i2c_master_transmit(bme_handle, (uint8_t[]){REG_CTRL_HUM, ctrl_hum}, 2, 100) != ESP_OK) return false;
    uint8_t ctrl_meas = (1 << 5) | (1 << 2) | 0x01;            // osrs_t=x1, osrs_p=x1, mode=forced(01)
    if (i2c_master_transmit(bme_handle, (uint8_t[]){REG_CTRL_MEAS, ctrl_meas}, 2, 100) != ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(10));   // час на вимірювання -- forced mode (Модуль 4.4, Частина 3)

    uint8_t raw[8];
    if (wtr(REG_PRESS_MSB, raw, sizeof(raw)) != ESP_OK) return false;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8)  |  (int32_t)raw[7];

    int32_t  T_int   = bme280_compensate_T(&calib, adc_T);   // ОБОВ'ЯЗКОВО першою -- рахує calib.t_fine
    uint32_t P_q24_8 = bme280_compensate_P(&calib, adc_P);
    uint32_t H_q22_10= bme280_compensate_H(&calib, adc_H);

    *temp_C    = T_int / 100.0f;
    *press_hPa = (P_q24_8 / 256.0f) / 100.0f;
    *hum_RH    = H_q22_10 / 1024.0f;
    return true;
}
```

**`components/env_i2c/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "env_i2c.c"
    INCLUDE_DIRS "include"
    REQUIRES driver bme280_compensate i2c_bus esp_timer log freertos
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`env_i2c_init()` більше не створює шину сама** — виклик
>   `i2c_new_master_bus()` переїхав у `i2c_bus` (Крок 1); тут лишається
>   лише `i2c_master_bus_add_device(i2c_bus_handle(), ...)`, який просто
>   реєструє BME280 як третього (разом з RTC і OLED, Крок 3) споживача вже
>   готової шини.
> - **`wtr()` — та сама функція-патерн, що `wtr_read_register()` з Модуля
>   4.3**, лише узагальнена на довільну довжину — один патерн WTR
>   обслуговує і Chip ID (1 байт), і калібрування (26+7 байт), і
>   вимірювання (8 байт).
> - **`BME280_ADDR = 0x76`, тому що `SDO`→GND** — пряме застосування
>   Модуля 4.2 (Додаток Б).

### Завдання 2.1
Прошийте (виклик `i2c_bus_init()` має передувати `env_i2c_init()` у
`app_main` — поки що додайте тимчасовий виклик обох плюс `env_i2c_read()`
+ `ESP_LOGI`), переконайтесь, що консоль щосекунди показує правдоподібні
`temp_C≈18-28`, `hum_RH≈20-70`, `press_hPa≈980-1030`.

### Завдання 2.2 — запитання для обговорення
Якби `env_i2c.c` використовував ту саму `static int32_t t_fine`, що й
приклад коду в Модулі 4.5 (глобальну, а не поле структури), і в проєкті
одночасно існував другий такий же файл для SPI-датчика з такою самою
глобальною змінною — що конкретно пішло б не так під час компіляції чи під
час виконання? Порівняйте з рішенням, прийнятим у `bme280_compensate.h`.

---

## 04 · Крок 3 · Tiny RTC + OLED: вовк-заставка й дашборд ⏱ 50 хв

> **🟢 МЕТА**
> Додати на ту саму I²C-шину ще два пристрої — Tiny RTC (DS1307) і
> SSD1306 — застосувавши напряму BCD-конвертацію й burst-read з Модуля 4.3,
> і показати на дисплеї той самий анімований вовк, що й у прикладі
> `u8g2_clock_and_wolf_logo_EXAMPLE.md`, лише тепер обмежений точно 5
> секундами, а після нього — живий дашборд із датчиками.

### Частина 1 — Tiny RTC (DS1307): burst-read часу

**`components/rtc_ds1307/include/rtc_ds1307.h`:**
```c
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint8_t sec, min, hour, wday, date, month, year; } rtc_time_t;

void rtc_ds1307_init(void);
bool rtc_ds1307_read_time(rtc_time_t *out);
void rtc_ds1307_set_from_tm(const struct tm *utc);   // одноразовий запис після SNTP-синхронізації (Крок 8)

#ifdef __cplusplus
}
#endif
```

**`components/rtc_ds1307/rtc_ds1307.c`:**
```c
#include "rtc_ds1307.h"
#include "i2c_bus.h"
#include "esp_log.h"

#define DS1307_ADDR    0x68     // 7-бітна адреса (Модуль 4.2/4.3)
#define I2C_FREQ_HZ    100000

static const char *TAG = "RTC_DS1307";
static i2c_master_dev_handle_t s_handle;

static uint8_t bcd_to_dec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t dec_to_bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

void rtc_ds1307_init(void)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = DS1307_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle(), &dev_cfg, &s_handle));
    ESP_LOGI(TAG, "Tiny RTC (DS1307) ready, address 0x%02X", DS1307_ADDR);
}

bool rtc_ds1307_read_time(rtc_time_t *out)
{
    uint8_t reg = 0x00, raw[7];
    // burst-read з auto-increment -- один "знімок" усіх 7 полів часу за одну транзакцію (Модуль 4.3, Частина 3)
    if (i2c_master_transmit_receive(s_handle, &reg, 1, raw, 7, 100) != ESP_OK) return false;

    out->sec   = bcd_to_dec(raw[0] & 0x7F);   // біт 7 -- CH (Clock Halt), відкидаємо
    out->min   = bcd_to_dec(raw[1]);
    out->hour  = bcd_to_dec(raw[2] & 0x3F);   // 24-годинний формат
    out->wday  = raw[3];                       // НЕ BCD -- просте число 1-7 (Модуль 4.3, Частина 3)
    out->date  = bcd_to_dec(raw[4]);
    out->month = bcd_to_dec(raw[5]);
    out->year  = bcd_to_dec(raw[6]);
    return true;
}

void rtc_ds1307_set_from_tm(const struct tm *utc)
{
    uint8_t buf[8];
    buf[0] = 0x00;                                        // адреса першого регістра (Seconds)
    buf[1] = dec_to_bcd(utc->tm_sec) & 0x7F;               // біт 7 (CH) = 0 -- запускає годинник, Модуль 4.2
    buf[2] = dec_to_bcd(utc->tm_min);
    buf[3] = dec_to_bcd(utc->tm_hour) & 0x3F;              // 24-годинний формат
    buf[4] = (uint8_t)(utc->tm_wday + 1);                  // struct tm: 0=нд..6=сб -> DS1307: 1=нд..7=сб
    buf[5] = dec_to_bcd(utc->tm_mday);
    buf[6] = dec_to_bcd(utc->tm_mon + 1);                  // struct tm: місяці 0-11
    buf[7] = dec_to_bcd(utc->tm_year % 100);
    esp_err_t err = i2c_master_transmit(s_handle, buf, sizeof(buf), 100);
    ESP_LOGI(TAG, "DS1307 updated with time from SNTP: %s", err == ESP_OK ? "OK" : "ERROR");
}
```

**`components/rtc_ds1307/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "rtc_ds1307.c"
    INCLUDE_DIRS "include"
    REQUIRES driver i2c_bus log
)
```

> **🟣 РОЗБІР: чому саме так**
> - **RTC, а не лише системний час ESP32-S3** — поки плата увімкнена,
>   системний годинник (Крок 8, SNTP) і так тримає коректний час без
>   жодного додаткового заліза. Перевага Tiny RTC — виключно в тому, що
>   вона **переживає відключення живлення**: DS1307 з батарейкою CR2032
>   продовжує рахувати секунди своїм власним кварцом навіть коли ESP32-S3
>   повністю знеструмлений, тоді як системний `time()` після кожного
>   `Reset`/втрати живлення починає з нуля, доки SNTP не встигне
>   синхронізуватись заново.
> - **`rtc_ds1307_set_from_tm()` викликається один раз, а не щоцикл** —
>   RTC потрібно лише "засіяти" правильним часом одного разу після
>   першого успішного SNTP-синку (Крок 8); далі DS1307 рахує час
>   самостійно, апаратно, без жодної допомоги від Wi-Fi чи CPU.

### Частина 2 — OLED (SSD1306): вовк-заставка та дашборд

> Ця частина продовжує HAL-міст `u8g2_hal.c/.h` з Модуля 4.2 (той самий
> код, що й генерує `setup_u8g2.sh`, і той самий git-компонент `u8g2`,
> підключений через `idf_component.yml`, що й у прикладі
> `u8g2_clock_and_wolf_logo_EXAMPLE.md`) — **з однією відмінністю від
> Модуля 4.2: тут HAL-міст живе у власному компоненті
> `components/u8g2_hal/`, а не серед файлів `main/`.** Причина —
> нижче, `oled_display` (а не лише `main`) теж має лінкуватись проти
> цього коду (`REQUIRES u8g2_hal` у його `CMakeLists.txt`), а
> ESP-IDF резолвить `REQUIRES` лише за іменами **компонентів**, не
> окремих файлів іншого компонента (див. ЕРАТА, п. 1).

**`components/u8g2_hal/include/u8g2_hal.h`:**
```c
#pragma once

#include "driver/i2c_master.h"
#include "u8g2.h"

// Прив'язує u8g2 callback-и до вже створеного I2C device handle (новий driver/i2c_master.h)
void u8g2_hal_set_i2c_device(i2c_master_dev_handle_t dev_handle);

uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
```

**`components/u8g2_hal/u8g2_hal.c`** (дослівно той самий HAL-міст, що
й у Модулі 4.2 — байтовий і GPIO/delay callback для `u8g2`, реалізовані
через `i2c_master_transmit()`):
```c
#include "u8g2_hal.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "U8G2_HAL";
static i2c_master_dev_handle_t s_dev_handle;

void u8g2_hal_set_i2c_device(i2c_master_dev_handle_t dev_handle)
{
    s_dev_handle = dev_handle;
}

uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    // u8g2 надсилає I2C-транзакцію шматками по 32 байти (SEND) між START/END_TRANSFER
    static uint8_t buffer[32];
    static uint8_t buf_idx;

    switch (msg) {
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;

        case U8X8_MSG_BYTE_SEND: {
            const uint8_t *data = (const uint8_t *)arg_ptr;
            for (uint8_t i = 0; i < arg_int && buf_idx < sizeof(buffer); i++) {
                buffer[buf_idx++] = data[i];
            }
            break;
        }

        case U8X8_MSG_BYTE_END_TRANSFER: {
            esp_err_t err = i2c_master_transmit(s_dev_handle, buffer, buf_idx, 100);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "i2c_master_transmit failed: %s", esp_err_to_name(err));
            }
            break;
        }

        case U8X8_MSG_BYTE_INIT:
        default:
            break;
    }
    return 1;
}

uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;

        case U8X8_MSG_DELAY_10MICRO:
            esp_rom_delay_us(10);
            break;

        case U8X8_MSG_DELAY_100NANO:
            esp_rom_delay_us(1);
            break;

        case U8X8_MSG_GPIO_AND_DELAY_INIT:
        case U8X8_MSG_GPIO_RESET:
        default:
            // апаратний I2C, окремі GPIO для reset/clock/data не використовуються
            break;
    }
    return 1;
}
```

**`components/u8g2_hal/CMakeLists.txt`:**
```cmake
idf_component_register(
                    SRCS "u8g2_hal.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        u8g2
                        driver
                        esp_rom
                        log
                        freertos
)
```

**`components/oled_display/include/oled_display.h`:**
```c
#pragma once
#include "rtc_ds1307.h"

#ifdef __cplusplus
extern "C" {
#endif

void oled_display_init(void);
void oled_play_wolf_boot_animation(void);   // блокує рівно ~5 секунд (Крок 3, за вимогою проєкту)
void oled_draw_dashboard(const rtc_time_t *t,
                          float t_i2c, float h_i2c, float p_i2c,
                          float t_spi, float h_spi, float p_spi,
                          float threshold_c, bool led_on, const char *led_source);

#ifdef __cplusplus
}
#endif
```

**`components/oled_display/oled_display.c`:**
```c
#include "oled_display.h"
#include "i2c_bus.h"
#include "u8g2.h"
#include "u8g2_hal.h"      // готовий HAL-міст з Модуля 4.2 (setup_u8g2.sh)
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define SSD1306_ADDR   0x3C
#define I2C_FREQ_HZ    400000

static i2c_master_dev_handle_t s_oled_handle;
static u8g2_t u8g2;

// -- бітмапи вовка, 32x32, XBM-порядок -- дослівно з Модуля 4.3,
// u8g2_clock_and_wolf_logo_EXAMPLE.md, Частина 2 (2 кадри для моргання) --
static const unsigned char wolf_logo_32x32_open[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x0C, 0x78, 0x00, 0x00, 0x1E,
    0xF8, 0x00, 0x00, 0x1F, 0xFC, 0x01, 0x80, 0x3F, 0xFC, 0x03, 0xC0, 0x3F,
    0xFC, 0x03, 0xC0, 0x3F, 0xFE, 0x07, 0xE0, 0x7F, 0xFE, 0x0F, 0xF0, 0x7F,
    0xFF, 0x1F, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x7F, 0xFC, 0x3F, 0xFE, 0x7F, 0xFC, 0x3F, 0xFE, 0x7F, 0xFC, 0x3F, 0xFE,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0x7F, 0xFE, 0xFF, 0xFF, 0x7F,
    0xFC, 0xFF, 0xFF, 0x3F, 0xF8, 0xFF, 0xFF, 0x1F, 0xF0, 0xFF, 0xFF, 0x0F,
    0xE0, 0xFF, 0xFF, 0x07, 0xC0, 0xFF, 0xFF, 0x03, 0x80, 0xFF, 0xFF, 0x01,
    0x00, 0x7F, 0xFE, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0xFC, 0x3F, 0x00,
    0x00, 0xF8, 0x1F, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xE0, 0x07, 0x00,
    0x00, 0xC0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00,
};
// Той самий силует, без вирізів-очей (рядки 12-14) -- "моргання" (очі заплющені)
static const unsigned char wolf_logo_32x32_blink[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x0C, 0x78, 0x00, 0x00, 0x1E,
    0xF8, 0x00, 0x00, 0x1F, 0xFC, 0x01, 0x80, 0x3F, 0xFC, 0x03, 0xC0, 0x3F,
    0xFC, 0x03, 0xC0, 0x3F, 0xFE, 0x07, 0xE0, 0x7F, 0xFE, 0x0F, 0xF0, 0x7F,
    0xFF, 0x1F, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0x7F, 0xFE, 0xFF, 0xFF, 0x7F,
    0xFC, 0xFF, 0xFF, 0x3F, 0xF8, 0xFF, 0xFF, 0x1F, 0xF0, 0xFF, 0xFF, 0x0F,
    0xE0, 0xFF, 0xFF, 0x07, 0xC0, 0xFF, 0xFF, 0x03, 0x80, 0xFF, 0xFF, 0x01,
    0x00, 0x7F, 0xFE, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0xFC, 0x3F, 0x00,
    0x00, 0xF8, 0x1F, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xE0, 0x07, 0x00,
    0x00, 0xC0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00,
};

void oled_display_init(void)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = SSD1306_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle(), &dev_cfg, &s_oled_handle));

    u8g2_hal_set_i2c_device(s_oled_handle);   // HAL-міст з Модуля 4.2 -- прив'язує u8g2 до цього дескриптора
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}

// Рівно ~5 секунд моргання (а не фіксована кількість циклів) -- час-обмежений
// цикл, щоб тривалість не "плила" залежно від того, скільки саме циклів
// встигло влізти при конкретній частоті шини.
void oled_play_wolf_boot_animation(void)
{
    const int64_t DURATION_US = 5 * 1000 * 1000;
    int64_t start = esp_timer_get_time();

    while (esp_timer_get_time() - start < DURATION_US) {
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawXBM(&u8g2, 48, 16, 32, 32, wolf_logo_32x32_open);    // 48=(128-32)/2, 16=(64-32)/2 -- по центру
        u8g2_SendBuffer(&u8g2);
        vTaskDelay(pdMS_TO_TICKS(500));

        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawXBM(&u8g2, 48, 16, 32, 32, wolf_logo_32x32_blink);
        u8g2_SendBuffer(&u8g2);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void oled_draw_dashboard(const rtc_time_t *t,
                          float t_i2c, float h_i2c, float p_i2c,
                          float t_spi, float h_spi, float p_spi,
                          float threshold_c, bool led_on, const char *led_source)
{
    char line_time[16], line_i2c[24], line_spi[24], line_status[24];   // [16], не [10] -- ЕРАТА п.4
    snprintf(line_time, sizeof(line_time), "%02u:%02u:%02u", t->hour, t->min, t->sec);
    snprintf(line_i2c, sizeof(line_i2c), "I2C %.1fC %.0f%% %.0fhPa", t_i2c, h_i2c, p_i2c);
    snprintf(line_spi, sizeof(line_spi), "SPI %.1fC %.0f%% %.0fhPa", t_spi, h_spi, p_spi);
    snprintf(line_status, sizeof(line_status), "Porig%.0fC LED:%s(%s)",
             threshold_c, led_on ? "ON" : "OFF", led_source);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);   // великий шрифт для часу -- та сама ідея, що в Модулі 4.3
    u8g2_DrawStr(&u8g2, 4, 26, line_time);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);         // менший шрифт -- три рядки показань і статусу
    u8g2_DrawStr(&u8g2, 0, 40, line_i2c);
    u8g2_DrawStr(&u8g2, 0, 52, line_spi);
    u8g2_DrawStr(&u8g2, 0, 63, line_status);
    u8g2_SendBuffer(&u8g2);
}
```

**`components/oled_display/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "include"
    REQUIRES driver i2c_bus rtc_ds1307 esp_timer log freertos
)
```
```yaml
# components/oled_display/idf_component.yml -- та сама git-залежність, що встановив
# setup_u8g2.sh у Модулі 4.2; скопіюйте секцію u8g2 з кореневого idf_component.yml сюди.
dependencies:
  olikraus/u8g2:
    git: https://github.com/olikraus/u8g2.git
```

> **🟣 РОЗБІР: чому саме так**
> - **`oled_play_wolf_boot_animation()` — цикл, обмежений часом
>   (`esp_timer_get_time()`), а не фіксованою кількістю ітерацій** — у
>   прикладі Модуля 4.3 було "3 моргання" без гарантії конкретної
>   тривалості; тут вимога проєкту точна ("перші 5 секунд"), тож цикл
>   перевіряє реальний минулий час на кожній ітерації й природно виконує
>   стільки циклів моргання (open/blink), скільки влазить рівно в 5
>   секунд — зазвичай 6-7 повних циклів при заданих затримках 500+150 мс.
> - **`oled_draw_dashboard()` бере вже готові показання як параметри, а не
>   сам викликає `env_i2c_read()`/`env_spi_read()`** — `oled_display` не
>   має `REQUIRES env_i2c`/`env_spi`: той самий принцип поділу
>   "periferія не знає про іншу periferію", що й у Кроці 1; хто саме читає
>   датчики і хто малює дисплей — дві різні відповідальності, об'єднані
>   лише в `main/app_main.c` (Крок 9).

### Завдання 3.1 (обов'язкове)
Прошийте повний код Кроку 3 (з `i2c_bus_init()` → `env_i2c_init()` →
`rtc_ds1307_init()` → `oled_display_init()` → `oled_play_wolf_boot_animation()`
у тимчасовому `app_main`). Переконайтесь, що на дисплеї одразу після
подачі живлення ≈5 секунд моргає мордочка вовка, а потім екран лишається
порожнім (`oled_draw_dashboard()` буде викликана в Кроці 9, коли всі дані
вже будуть готові).

### Завдання 3.2
Тимчасово запишіть у DS1307 довільний час через `rtc_ds1307_set_from_tm()`
(створіть `struct tm` вручну з довільними полями) і переконайтесь, що
`rtc_ds1307_read_time()` після цього повертає саме ці значення. Це
підтверджує механізм "SNTP → RTC" з Кроку 8 ще до того, як там з'явиться
реальний Wi-Fi.

### Завдання 3.3 — запитання для обговорення
У Модулі 4.3 три пристрої на спільній шині були SSD1306, DS1307 і EEPROM
AT24C32E. У цьому проєкті EEPROM відсутній, а замість нього — другий
BME280 (I²C). Чому фізично можливо мати одночасно "три I²C-пристрої на
GPIO8/9" (Крок 3) і "SPI-пристрій на зовсім інших GPIO" (Крок 5) на тій
самій платі — що саме дозволяє цим двом групам periferії ніяк не заважати
одна одній?

---

## 05 · Крок 4 · I²C: обробка помилок NACK, діагностика ⏱ 25 хв

> **🟢 МЕТА**
> Побачити "гучну" I²C-помилку (NACK) навмисно й навчитись відрізняти її
> від "тихих" помилок даних (Модуль 4.3: BCD/Big-Endian) — на реальному
> датчику логера, а не на абстрактному прикладі.

**Тимчасовий діагностичний код (додати в `main/app_main.c` для цього кроку):**
```c
#include "esp_err.h"

void i2c_error_demo(void)
{
    float t, p, h;
    for (int i = 0; i < 5; i++) {
        bool ok = env_i2c_read(&t, &p, &h);
        ESP_LOGI("I2C_DIAG", "Attempt %d: %s", i, ok ? "success" : "ERROR (NACK/timeout)");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### Завдання 4.1 (обов'язкове)
Тимчасово від'єднайте дріт `SDA` від BME280 №1 (лишіть живлення й `SCL`) і
викличте `i2c_error_demo()`. Переконайтесь, що `env_i2c_read()` повертає
`false` на кожній спробі, а `rtc_ds1307_read_time()`/дисплей у цей момент
**теж** перестають отримувати нові дані (спільна шина, Крок 3) — хоча RTC і
OLED фізично справні. Поверніть `SDA` на місце.

### Завдання 4.2
Порівняйте поведінку з Завдання 4.1 з тим, що сталось би, якби замість
обриву `SDA` хтось випадково переплутав біти калібрувальних коефіцієнтів
(як `dig_H4`/`dig_H5` у Модулі 4.4). Яка з двох категорій помилок дасть
`env_i2c_read() == false` одразу на консолі, а яка — правдоподібне, але
неправильне число без жодного `false`?

### Завдання 4.3 — запитання для обговорення
`env_i2c_read()` повертає `bool`, а не просто `void` з логуванням помилки
всередині. Як цей `bool` буде використаний у Кроці 9 (головний цикл) — що
логер повинен зробити з показанням, якщо `env_i2c_read()` повернула
`false` саме в той момент, коли йде публікація в MQTT і малювання
дашборда на дисплеї?

### Відповіді
**4.1.** Кожна спроба логує `ПОМИЛКА (NACK/timeout)` — без фізичного `SDA`
Master не може навіть завершити фазу адресації (`START`+`0x76`+`W`), тож
`i2c_master_transmit()` у `env_i2c_read()` повертає код помилки вже на
першому внутрішньому виклику. При цьому `rtc_ds1307_read_time()` не
торкається тієї самої лінії `SDA` фізично інакше — обрив `SDA` ламає шину
**для всіх** пристроїв одночасно (лінія спільна, Крок 0/3), тож і RTC, і
дисплей теж не зможуть спілкуватись, попри те, що самі вони справні й
підключені коректно.

**4.3.** Головний цикл (Крок 9) не повинен публікувати недостовірне
показання як реальне й не повинен малювати його на дисплеї як справжнє —
типове рішення: пропустити це поле в JSON цього циклу, а на дисплеї або
лишити попереднє (останнє валідне) значення, або явно позначити "---"
замість числа — те саме розрізнення "гучної" і "тихої" відмови, що й у
Проєкті 7, лише тепер застосоване одразу до двох виходів (мережа й екран).

---

## 06 · Крок 5 · SPI: BME280 Mode 0, Chip ID, калібрування ⏱ 40 хв

> **🟢 МЕТА**
> Прочитати другий, фізично окремий BME280 по SPI (Модуль 4.4) — той самий
> `bme280_compensate`, зовсім інший транспорт, повністю незалежні GPIO від
> I²C-шини з Кроків 2-4.

**`components/env_spi/include/env_spi.h`:**
```c
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void env_spi_init(void);
bool env_spi_read(float *temp_C, float *press_hPa, float *hum_RH);

#ifdef __cplusplus
}
#endif
```

**`components/env_spi/env_spi.c`:**
```c
#include "env_spi.h"
#include "bme280_compensate.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define PIN_SCLK       5
#define PIN_MOSI       6
#define PIN_MISO       16
#define PIN_CS         17
#define SPI_HOST_USED  SPI3_HOST

#define REG_ID         0xD0
#define REG_CTRL_HUM   0xF2
#define REG_CTRL_MEAS  0xF4
#define REG_PRESS_MSB  0xF7

static const char *TAG = "ENV_SPI";
static spi_device_handle_t bme;
static bme280_calib_t calib;

// Спосіб А (комбінована транзакція) -- Модуль 4.4, Частина 2: узагальнена burst-читалка
static esp_err_t bme280_read_regs(uint8_t reg, uint8_t *out, size_t len)
{
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    tx[0] = reg | 0x80;               // read-біт
    memset(&tx[1], 0x00, len);
    spi_transaction_t t = { .length = 8 * (1 + len), .tx_buffer = tx, .rx_buffer = rx };
    esp_err_t err = spi_device_polling_transmit(bme, &t);
    if (err == ESP_OK) memcpy(out, &rx[1], len);
    return err;
}

static esp_err_t bme280_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), value };   // write-біт = 0
    spi_transaction_t t = { .length = 16, .tx_buffer = tx };
    return spi_device_polling_transmit(bme, &t);
}

void env_spi_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI, .miso_io_num = PIN_MISO, .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_USED, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,   // 1 МГц -- безпечний старт (Модуль 4.4)
        .mode = 0,                            // CPOL=0, CPHA=0 -- обов'язково для BME280
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_USED, &devcfg, &bme));

    uint8_t chip_id = 0;
    ESP_ERROR_CHECK(bme280_read_regs(REG_ID, &chip_id, 1));
    ESP_LOGI(TAG, "SPI BME280 id=0x%02X (awaiting 0x60)", chip_id);

    // reset + затримка -- копіювання NVM->робочі регістри (Модуль 4.5, Частина 2)
    bme280_write_reg(0xE0, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t buf_tp[26], buf_h[7];
    ESP_ERROR_CHECK(bme280_read_regs(0x88, buf_tp, sizeof(buf_tp)));
    ESP_ERROR_CHECK(bme280_read_regs(0xE1, buf_h, sizeof(buf_h)));
    bme280_parse_calib(&calib, buf_tp, buf_h);

    ESP_LOGI(TAG, "SPI ready: SCLK=%d MOSI=%d MISO=%d CS=%d", PIN_SCLK, PIN_MOSI, PIN_MISO, PIN_CS);
}

bool env_spi_read(float *temp_C, float *press_hPa, float *hum_RH)
{
    if (bme280_write_reg(REG_CTRL_HUM, 0x01) != ESP_OK) return false;                    // osrs_h=x1
    uint8_t ctrl_meas = (1 << 5) | (1 << 2) | 0x01;                                       // forced mode
    if (bme280_write_reg(REG_CTRL_MEAS, ctrl_meas) != ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t raw[8];
    if (bme280_read_regs(REG_PRESS_MSB, raw, sizeof(raw)) != ESP_OK) return false;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8)  |  (int32_t)raw[7];

    int32_t  T_int    = bme280_compensate_T(&calib, adc_T);
    uint32_t P_q24_8  = bme280_compensate_P(&calib, adc_P);
    uint32_t H_q22_10 = bme280_compensate_H(&calib, adc_H);

    *temp_C    = T_int / 100.0f;
    *press_hPa = (P_q24_8 / 256.0f) / 100.0f;
    *hum_RH    = H_q22_10 / 1024.0f;

    // SPI, на відміну від I2C, не має апаратного ACK/NACK -- транзакція
    // "успішно" повертає esp_err_t == ESP_OK навіть якщо CSB/MISO не
    // підключені фізично (майстер сам генерує тактовий сигнал, а на
    // відключеному MISO читає лише підтягнуті вгору "1"). Тому окремо
    // перевіряємо, що показання фізично правдоподібне -- інакше нижче по
    // ланцюгу (MQTT-телеметрія, дашборд) отримали б "справжнє" false
    // показання замість чесного false від цієї функції.
    if (*temp_C < -20.0f || *temp_C > 60.0f) return false;

    return true;
}
```

**`components/env_spi/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "env_spi.c"
    INCLUDE_DIRS "include"
    REQUIRES driver bme280_compensate esp_timer log freertos
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`env_spi.c` і `env_i2c.c` мають майже ідентичну структуру
>   `env_*_read()`**, окрім того, як саме передаються байти на шину — це
>   пряме підтвердження висновку Модуля 4.4 (Додаток А): різниця I²C/SPI —
>   у транспорті, а не в логіці роботи з самим BME280.
> - **`env_spi` не має `REQUIRES i2c_bus`** — і не повинен: це зовсім
>   інша, фізично незалежна шина (Крок 0), власний `spi_bus_initialize()`
>   на власних GPIO.
> - **`env_spi_read()` перевіряє не лише `esp_err_t`, а й правдоподібність
>   значення** (`return false`, якщо температура поза `[-20°C, 60°C]`) —
>   бо, на відміну від I²C, у SPI немає апаратного ACK/NACK: майстер сам
>   генерує тактовий сигнал і "успішно" отримує байти, навіть коли `CS`/
>   `MISO` фізично не підключені (лінія просто висить у "1" від підтяжки).
>   Без цієї перевірки відключений дріт виглядав би як `spi_ok: true` з
>   температурою рівно `0.0°C` — детальніше в Кроці 9, самотест
>   `run_wiring_self_test()`, де саме ця різниця I²C/SPI й спливла на
>   практиці.

### Завдання 5.1
Прошийте, переконайтесь, що консоль показує `SPI BME280 id=0x60 -- OK`, і
що `env_spi_read()` дає схожі значення до тих, що дає `env_i2c_read()` з
Кроку 2.

### Завдання 5.2 — запитання для обговорення
Порівняйте кількість фізичних дротів (окрім живлення) між ESP32-S3 і кожним
з двох BME280: скільки для I²C-варіанту (Крок 2), скільки для SPI-варіанту
(цей крок)? Якби до проєкту треба було додати третій, четвертий, п'ятий
BME280, у якому варіанті (I²C чи SPI) кількість зайнятих GPIO зростала б
швидше?

---

## 07 · Крок 6 · SPI: цілочисельна компенсація вже готова — перевірка порядку T→P→H ⏱ 35 хв

> **🟢 МЕТА**
> `bme280_compensate.c` (Крок 2) уже реалізує офіційні цілочисельні формули
> Bosch (Модуль 4.5, Частина 3). Тут — не новий код, а експериментальна
> перевірка того самого правила "T обов'язково першою", тепер на реальному
> SPI-датчику логера.

### Завдання 6.1 — запитання для обговорення
Модуль 4.5 (Частина 3, Завдання 3.2) показав: якщо викликати
`bme280_compensate_P()` до `bme280_compensate_T()`, тиск порахується на
**застарілому** `t_fine` з попереднього циклу. У нашому `env_spi_read()`
(Крок 5) порядок викликів `T_int` → `P_q24_8` → `H_q22_10` уже правильний.
Знайдіть рядок коду, який гарантує цей порядок, і поясніть, що конкретно
станеться з `press_hPa`, якщо хтось у майбутньому рефакторингу поміняє два
сусідні рядки місцями.

### Завдання 6.2
Логічним аналізатором (за наявності, Модуль 4.5, Частина 4) захопіть
трафік `env_spi_read()`: підтвердьте на реальному сигналі Mode 0 (CPOL=0,
CPHA=0), MSB-first, і що `CS` лишається безперервно `LOW` протягом усієї
8-байтної burst-read транзакції `0xF7`.

### Відповідь
**6.1.** Порядок гарантує послідовність трьох рядків у тілі
`env_spi_read()`: `T_int = bme280_compensate_T(&calib, adc_T);` виконується
першим рядком з трьох, і саме цей виклик записує `calib.t_fine`, який
потім читають наступні два виклики. Якби рядки помінялись місцями,
`bme280_compensate_P()` прочитала б `calib.t_fine` зі **старого**
виклику — тиск вийшов би в правдоподібному, але систематично зміщеному
діапазоні, без жодної помилки компіляції чи `esp_err_t`.

---

## 08 · Крок 7 · DMA: ADC continuous — контрольна ручка порогу без навантаження CPU ⏱ 40 хв

> **🟢 МЕТА**
> Зчитувати потенціометр (поріг тривоги) через `adc_continuous` API
> (Модуль 4.6) — periferія сама, у фоні, наповнює кільце DMA-буферів;
> задача лише забирає вже готові блоки, жодного циклу опитування АЦП.

**`components/ctrl_adc/include/ctrl_adc.h`:**
```c
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     ctrl_adc_init(void);
float    ctrl_adc_get_threshold_c(void);   // поточний поріг тривоги, °C, у діапазоні 15.0-35.0
uint32_t ctrl_adc_get_raw(void);           // сирий усереднений відлік АЦП, 0-4095 (для дашборда/діагностики)

#ifdef __cplusplus
}
#endif
```

**`components/ctrl_adc/ctrl_adc.c`:**
```c
#include "ctrl_adc.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_3      // GPIO4 -- та сама ручка, що й у Модулі 4.6
#define ADC_ATTEN      ADC_ATTEN_DB_12
#define ADC_BIT_WIDTH  ADC_BITWIDTH_12

#define FRAME_SIZE     256
#define POOL_FRAMES    4

#define THRESH_MIN_C   15.0f
#define THRESH_MAX_C   35.0f

static const char *TAG = "CTRL_ADC";
static adc_continuous_handle_t adc_handle;
static TaskHandle_t adc_task_handle;
static volatile float s_threshold_c = 25.0f;   // безпечне значення за замовчуванням до першого блоку
static volatile uint32_t s_adc_raw = 0;        // останній усереднений відлік АЦП, 0-4095

static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t must_yield = pdFALSE;
    vTaskNotifyGiveFromISR(adc_task_handle, &must_yield);
    return must_yield == pdTRUE;
}

static void adc_read_task(void *arg)
{
    uint8_t result[FRAME_SIZE];
    uint32_t bytes_read;

    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // спати, поки DMA не наповнить черговий блок

        esp_err_t err = adc_continuous_read(adc_handle, result, FRAME_SIZE, &bytes_read, 0);
        if (err != ESP_OK) continue;

        adc_digi_output_data_t *data = (adc_digi_output_data_t *)result;
        int count = bytes_read / sizeof(adc_digi_output_data_t);
        uint32_t sum = 0;
        for (int i = 0; i < count; i++) sum += data[i].type2.data;
        uint32_t avg_raw = sum / count;                     // усереднення блоку -- згладжує шум ручки

        float frac = avg_raw / 4095.0f;                     // 0.0 .. 1.0
        s_threshold_c = THRESH_MIN_C + frac * (THRESH_MAX_C - THRESH_MIN_C);
        s_adc_raw = avg_raw;
    }
}

void ctrl_adc_init(void)
{
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = FRAME_SIZE * POOL_FRAMES,
        .conv_frame_size    = FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));

    adc_digi_pattern_config_t pattern[1] = {
        { .atten = ADC_ATTEN, .channel = ADC_CHANNEL, .unit = ADC_UNIT, .bit_width = ADC_BIT_WIDTH },
    };
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20000,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num    = 1,
        .adc_pattern    = pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));

    adc_continuous_evt_cbs_t cbs = { .on_conv_done = adc_conv_done_cb };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));   // 3-й арг. -- ЕРАТА п.2

    xTaskCreate(adc_read_task, "adc_read_task", 4096, NULL, 5, &adc_task_handle);
    ESP_LOGI(TAG, "ADC1 continuous+DMA ready: GPIO4, threshold %.0f-%.0f`C", THRESH_MIN_C, THRESH_MAX_C);
}

float ctrl_adc_get_threshold_c(void)
{
    return s_threshold_c;   // головний цикл (Крок 9) лише ЧИТАЄ -- жодного опитування АЦП з його боку
}

uint32_t ctrl_adc_get_raw(void)
{
    return s_adc_raw;
}
```

**`components/ctrl_adc/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "ctrl_adc.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_timer log freertos
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`ctrl_adc_get_threshold_c()` — це вся "публічна" поверхня компонента
>   для головного циклу** — весь DMA-потік і обробка блоків живуть
>   виключно всередині `adc_read_task`, повністю прихованому фоновому
>   потоці.
> - **`s_threshold_c` — `volatile float`**, записується однією задачею
>   (`adc_read_task`) і читається іншою (головний цикл, Крок 9) — для
>   одного `float` на цій платформі це прийнятне спрощення.

### Завдання 7.1 (обов'язкове)
Прошийте, тимчасово додайте `ESP_LOGI` з `ctrl_adc_get_threshold_c()` у
головний цикл `app_main`. Крутіть ручку потенціометра, переконайтесь, що
поріг плавно рухається між 15 і 35°C.

### Завдання 7.2 — запитання для обговорення
Порівняйте цей компонент з `range_sensor` із Проєкту 6 (Розділ III,
`adc_oneshot` — читання за запитом, без DMA). Що конкретно довелось би
змінити в головному циклі Проєкту 6, якби `range_sensor` теж використовував
`adc_continuous`+DMA замість `adc_oneshot`?

---

## 09 · Крок 8 · Wi-Fi provisioning + MQTT: SoftAP, власний брокер, pub/sub ⏱ 55 хв

> **🟢 МЕТА**
> Розібрати MQTT з нуля, підняти ESP32-S3 як тимчасову точку доступу, з якої
> телефон передає реальні дані вашого Wi-Fi (без жодного хардкоду SSID/
> пароля в коді), і підключити логер до **власного MQTT-брокера
> (Mosquitto)**, піднятого в тій самій локальній мережі — publish
> телеметрії й subscribe на команду керування LED, без хмари й без TLS.

### Перед стартом: чому MQTT "легковаговий" (пояснення "на пальцях")

**Проблема, яку вирішує MQTT.** HTTP-запит (`GET`/`POST`) — це повноцінний,
відносно "важкий" текстовий протокол: кожен запит несе URL, версію
протоколу, набір текстових заголовків — типово сотні байтів **накладних
витрат** навіть для корисного навантаження в кілька байтів. Для
мікроконтролера на батарейці, що раз на кілька секунд надсилає одне число
температури через повільний чи дорогий канал, ця накладна вартість —
суттєва частка всього трафіку й енергоспоживання.

**MQTT-рішення: мінімальний бінарний заголовок.** Найпростіший
MQTT-пакет (`PUBLISH` з QoS 0) має фіксований заголовок усього **2 байти**
плюс змінна довжина рядка теми. Сам протокол працює поверх TCP (одне
стабільне з'єднання тримається відкритим довго, а не встановлюється заново
на кожне повідомлення) — саме поєднання "мінімальний заголовок" + "довге
з'єднання" і дає MQTT репутацію "легковагового" протоколу для IoT.

**Publish/Subscribe: розв'язка видавця й підписника.** MQTT вводить
третього учасника — **брокер** — і повністю розв'язує видавців
(`publishers`) від підписників (`subscribers`): видавець публікує
повідомлення в **тему** (`topic`), нічого не знаючи, скільки підписників
її зараз слухають; підписник підписується на тему, нічого не знаючи, хто
саме туди щось опублікує.

**Аналогія: дошка оголошень, а не телефонний дзвінок.** HTTP-запит —
телефонний дзвінок: обидві сторони мають бути "на лінії" одночасно.
MQTT-тема — дошка оголошень: хтось приколює оголошення, не знаючи, хто
його прочитає; будь-хто зацікавлений підходить до дошки заздалегідь і
читає кожне нове оголошення — видавець і читач ніколи не мають бути
присутні "одночасно".

**Топіки та ієрархія.** Теми — рядки з ієрархією через `/`, наприклад
`logger/telemetry` чи `logger/control/led`. Підписники можуть
використовувати `+`/`#` як символи підстановки — у цьому проєкті підписка
проста, без wildcard-ів.

**QoS одним реченням.** `QoS 0` ("щонайбільше раз"), `QoS 1` ("щонайменше
раз" — з підтвердженням), `QoS 2` ("рівно раз" — найважчий). У цьому
проєкті телеметрія й команди керування йдуть з `QoS 1`.

### Частина 0 — Wi-Fi Provisioning: ESP32-S3 як SoftAP-хост, телефон як налаштувальник

**Чому не хардкодити SSID/пароль у коді.** До цього кроку єдиний спосіб
задати Wi-Fi-мережу — вписати рядок прямо в `main.c` й перепрошити плату.
Для реального пристрою (не для одного макета на столі розробника) це
неприйнятно: пароль потрапляє у вихідний код і в кожну прошивку, а зміна
мережі вимагає повторного підключення USB. ESP-IDF вирішує це компонентом
`wifi_provisioning`: пристрій, у якого ще немає збережених облікових даних,
сам на короткий час стає **точкою доступу** (SoftAP) із власною,
тимчасовою назвою мережі; користувач підключає до неї телефон, у
застосунку передає реальні SSID/пароль домашньої мережі через
зашифровану сесію (`protocomm`), ESP32-S3 зберігає їх у NVS і переходить у
звичайний STA-режим. При наступних увімкненнях `wifi_prov_mgr_is_provisioned()`
бачить, що дані вже є, і одразу підключається — SoftAP більше не виникає.

**Дві фази, один результат: пристрій підключається до вашого реального
роутера.** SoftAP тут — не мережа, у якій живе весь проєкт, а лише
короткий, службовий канал для передачі одного секрету:

```
Фаза 1 (лічені хвилини, лише при першому запуску)
  Телефон  ──Wi-Fi──▶  ESP32-S3 (сам є точкою доступу "PROV_iot_logger_XXXX")
     ЗАСТОСУНОК передає SSID+пароль ВАШОГО роутера через зашифровану protocomm-сесію

Фаза 2 (увесь інший час життя пристрою)
  ESP32-S3 (звичайний STA-клієнт)  ──Wi-Fi──▶  ВАШ РОУТЕР ──▶ MQTT-брокер (Mosquitto, Крок 8)
     ESP32 отримує IP від роутера так само, як будь-який ноутбук чи телефон у цій мережі
```

`wifi_prov_mgr` сам, без додаткового коду, виконує перехід між фазами:
щойно застосунок надсилає `WIFI_PROV_CRED_RECV`, менеджер сам викликає
`esp_wifi_set_config()` зі STA-даними й ініціює підключення — саме тому
`event_handler` (нижче) реагує на `IP_EVENT_STA_GOT_IP` однаково незалежно
від того, прийшло воно одразу після провіжинінгу чи при звичайному
перезавантаженні вже налаштованого пристрою.

**`components/wifi_prov/include/wifi_prov.h`:**
```c
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void wifi_prov_init_and_connect(void);   // блокує до отримання IP -- через SoftAP-provisioning або одразу як STA

#ifdef __cplusplus
}
#endif
```

**`components/wifi_prov/wifi_prov.c`:**
```c
#include "wifi_prov.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"   // множина -- ЕРАТА п.3
#include <stdio.h>

// Якщо збережені в NVS дані не підключають плату за цей час -- вважаємо їх
// неробочими (застаріла мережа/пароль), скидаємо провіжинінг і йдемо в
// SoftAP заново, щоб дані можна було ввести правильно ще раз (ЕРАТА п.7).
#define STA_CONNECT_TIMEOUT_MS   300000   // 5 хв -- з великим запасом, поки з'ясовуємо саму причину повільного STA

static const char *TAG = "WIFI_PROV";
static EventGroupHandle_t s_event_group;
static const int CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_PROV_EVENT) {
        switch (id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started -- connect your phone to this device's Wi-Fi AP");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *cfg = (wifi_sta_config_t *)data;
            ESP_LOGI(TAG, "Received credentials: SSID=%s", (const char *)cfg->ssid);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
            ESP_LOGE(TAG, "Provisioning failed -- check SSID/password and retry in the app");
            break;
        case WIFI_PROV_END:
            wifi_prov_mgr_deinit();   // сервіс SoftAP більше не потрібен -- звільняємо ресурси
            break;
        default:
            break;
        }
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_event_group, CONNECTED_BIT);
    }
}

void wifi_prov_init_and_connect(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    s_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();   // потрібен лише на час самого SoftAP-provisioning

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    wifi_prov_mgr_config_t prov_cfg = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_cfg));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned) {
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        char service_name[24];
        snprintf(service_name, sizeof(service_name), "PROV_iot_logger_%02X%02X", mac[4], mac[5]);

        ESP_LOGI(TAG, "Not provisioned -- starting SoftAP \"%s\"", service_name);
        // WIFI_PROV_SECURITY_1: сесія захищена proof-of-possession-рядком, а не відкрита точка доступу
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, "iot_logger_pop",
                                                          service_name, NULL));

        // Тут чекаємо без таймауту -- це нормальний стан "нікуди не поспішаємо,
        // чекаємо, поки хтось відкриє застосунок на телефоні".
        ESP_LOGI(TAG, "Waiting for IP...");
        xEventGroupWaitBits(s_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    } else {
        ESP_LOGI(TAG, "Already provisioned -- connecting as a regular STA");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "Waiting for IP with stored credentials (timeout %d s)...",
                 STA_CONNECT_TIMEOUT_MS / 1000);
        EventBits_t bits = xEventGroupWaitBits(s_event_group, CONNECTED_BIT, pdFALSE, pdTRUE,
                                                pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS));
        if (!(bits & CONNECTED_BIT)) {
            ESP_LOGW(TAG, "Stored Wi-Fi credentials did not connect in time -- resetting "
                          "provisioning and rebooting into SoftAP so they can be re-entered");
            wifi_prov_mgr_reset_provisioning();
            esp_restart();
        }
        wifi_prov_mgr_deinit();   // підключились самі -- сервіс провіжинінгу більше не потрібен
    }

    ESP_LOGI(TAG, "Wi-Fi ready");
}
```

> **🔴 ЕРАТА (п.7)** — оригінальний варіант цієї функції чекав IP
> (`xEventGroupWaitBits(..., portMAX_DELAY)`) **нескінченно**, без
> таймауту. На практиці це створює пастку: якщо плата вже була
> провіжинена раніше (в NVS збережені SSID/пароль), а мережа з тих пір
> змінилась, пароль ввели неправильно, або роутер зараз просто
> вимкнений — `wifi_prov_mgr_is_provisioned()` поверне `true`, код піде
> одразу в STA-гілку, спроби `esp_wifi_connect()` мовчки провалюються
> одна за одною (обробник `WIFI_EVENT_STA_DISCONNECTED` сам перевикликає
> `esp_wifi_connect()`, нічого не логуючи), і `xEventGroupWaitBits`
> висить вічно — у консолі повна тиша, а точка доступу
> `PROV_iot_logger_XXXX` для повторного налаштування вже **ніколи** не
> з'явиться сама. Єдиним виходом раніше було вручну стерти розділ NVS
> (`python -m esptool --chip esp32s3 -p COMx erase_region 0x9000 0x6000`,
> офсет/розмір беруться з таблиці розділів у логах завантаження).
> Виправлення додає `STA_CONNECT_TIMEOUT_MS` (спершу 60 с, потім піднято
> до 5 хв — див. нижче, чому): якщо за цей час IP
> не отримано, плата сама викликає `wifi_prov_mgr_reset_provisioning()`
> (очищає збережені дані) і `esp_restart()` — і на наступному колі
> знову піднімає SoftAP, готова прийняти правильні дані ще раз. Значення
> навмисно велике (не 10-15 с) — щоб не спрацьовувати хибно на звичайну
> затримку підключення (особливо одразу після повного power-cycle плати,
> коли й роутеру, і самій плати потрібен час на "прокидання"), а
> реагувати лише на дійсно неробочі облікові дані.

**`components/wifi_prov/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "wifi_prov.c"
    INCLUDE_DIRS "include"
    REQUIRES wifi_provisioning esp_wifi esp_netif esp_event nvs_flash log freertos
)
```

> **🟣 РОЗБІР: чому саме так**
> - **SoftAP, а не BLE-транспорт `protocomm`** — обидва підтримуються тим
>   самим `wifi_prov_mgr`; SoftAP обраний тут, бо не вимагає від телефона
>   підтримки BLE-провіжинінгу і працює однаково передбачувано на Android
>   і iOS через застосунок **ESP SoftAP Provisioning** (офіційний, Google
>   Play / App Store, Espressif) — той самий протокол, лише інший фізичний
>   канал, ніж у BLE-варіанта (**ESP BLE Provisioning**).
> - **`WIFI_PROV_SECURITY_1` з `pop`-рядком, а не `WIFI_PROV_SECURITY_0`**
>   — Security 0 передає SSID/пароль по SoftAP **у відкритому вигляді**;
>   Security 1 шифрує сесію `protocomm`, використовуючи `pop` ("proof of
>   possession") як спільний секрет — застосунок повинен ввести той самий
>   рядок, що зашитий тут у прошивці, інакше сесія не встановиться.
> - **`wifi_prov_mgr_is_provisioned()` перевіряється щоразу при старті** —
>   облікові дані зберігаються в NVS (тій самій, що й для звичайного
>   `esp_wifi_set_config`), тож SoftAP піднімається рівно один раз у житті
>   пристрою (доки хтось явно не стере NVS) — саме та поведінка, яку
>   очікує кінцевий користувач: не перевіджинюватись при кожному
>   перезавантаженні.
> - **`esp_netif_create_default_wifi_ap()` викликається завжди**, навіть
>   якщо пристрій уже провіжинений і SoftAP не підніметься — це лише
>   реєстрація мережевого інтерфейсу в стеку `esp_netif`, не активація
>   самої точки доступу; активує AP вже сам `wifi_prov_mgr_start_provisioning()`,
>   і лише коли він реально викликаний.

### Завдання 8.0.1 (обов'язкове)
Встановіть на телефон **ESP SoftAP Provisioning** (Android чи iOS,
розробник Espressif). Прошийте логер (поки що можна тимчасово зупинити
`app_main` одразу після `wifi_prov_init_and_connect()` і подивитись лог).
На **першому** запуску переконайтесь, що в списку Wi-Fi-мереж телефону
з'являється `PROV_iot_logger_XXXX`; підключіться до неї, відкрийте
застосунок, введіть `pop`-рядок `iot_logger_pop`, передайте SSID/пароль
вашої реальної мережі. Переконайтесь, що консоль показує `Wi-Fi готовий`.

### Завдання 8.0.2
Перезавантажте плату (кнопка `RESET`, без стирання flash). Переконайтесь,
що `PROV_iot_logger_XXXX` більше **не** з'являється в списку мереж
телефону — пристрій одразу підключається як STA, використовуючи облікові
дані з NVS.

### Завдання 8.0.3 — запитання для обговорення
Якби замість `wifi_prov_mgr` ви й далі передавали `wifi_ssid`/`wifi_pass`
як рядкові літерали в `main.c` (як у Кроках 1-7 цього документа до цього
моменту), що конкретно довелось би робити щоразу, коли пристрій переїжджає
в іншу Wi-Fi-мережу (наприклад, з домашньої лабораторії в аудиторію)? Чому
провіжинінг через телефон — це не "зручність заради зручності", а вимога
для будь-якого пристрою, який зрештою піде до кінцевого користувача, що не
має доступу до вихідного коду й USB-кабеля?

---

### Частина 1 — Піднімаємо власний MQTT-брокер (Mosquitto)

Жодної хмари цей проєкт не потребує: **Mosquitto**, запущений на
звичайному ПК чи Raspberry Pi в тій самій Wi-Fi мережі, куди щойно
підключився логер (Частина 0), — це повноцінний брокер для publish/subscribe
з довільною кількістю підписників (телефон, ноутбук, інший ESP32).

**Встановлення (вручну).**
```bash
# Linux (Debian/Ubuntu):
sudo apt install mosquitto mosquitto-clients

# macOS (Homebrew):
brew install mosquitto

# Windows -- офіційний інсталятор з https://mosquitto.org/download/,
# АБО (швидше, без діалогових вікон) через winget з PowerShell:
winget install --id EclipseFoundation.Mosquitto -e --accept-package-agreements --accept-source-agreements
```

> **🟠 МІНІ-ЗАВДАННЯ**
> Mosquitto 2.x за замовчуванням **відхиляє анонімні з'єднання** — потрібен
> мінімальний конфіг-файл. Створіть `mosquitto.conf` (для проєкту одразу з
> двома listener'ами — `1883` для прошивки й `9001`/`websockets` для
> веб-консолі з Челенджу 7, щоб не переналаштовувати брокер вдруге):
> ```
> allow_anonymous true
>
> listener 1883
>
> listener 9001
> protocol websockets
> ```
> і запустіть брокер саме з ним: `mosquitto -c mosquitto.conf -v`
> (`-v` — verbose, щоб бачити кожне підключення в консолі під час
> налагодження).

**Встановлення й запуск одним скриптом (автоматизація).** У репозиторії
проєкту є `broker/setup_broker.sh` — той самий принцип, що й
`setup_u8g2.sh` з Модуля 4.2: сам перевіряє, чи Mosquitto вже
встановлений (winget/apt/brew — залежно від ОС), сам визначає IP-адресу
цього комп'ютера в локальній мережі й одразу підказує, що вписати в
`idf.py menuconfig` і в `dashboard.html`, і сам запускає брокер із
`broker/mosquitto.conf` (готовий конфіг із двома listener'ами вище вже
лежить поруч зі скриптом):
```bash
cd iot_logger
./broker/setup_broker.sh              # встановити (якщо треба) + запустити
./broker/setup_broker.sh --no-install # лише запустити, не чіпати встановлення
```
Скрипт лишається на передньому плані (як і ручний `mosquitto -c ... -v`)
— саме так ви одразу бачите кожне підключення (плата, веб-консоль,
`mosquitto_sub`/`mosquitto_pub`) наживо в тому самому терміналі; `Ctrl+C`
зупиняє брокер.

**Дізнайтесь IP-адресу цього комп'ютера в локальній мережі** (`ipconfig`
на Windows, `ip addr` / `ifconfig` на Linux/macOS, або просто прочитайте
вивід `setup_broker.sh` вище) — саме на неї, а не на `localhost`,
підключатиметься ESP32-S3, бо це окремий фізичний пристрій у тій самій
Wi-Fi мережі.

**Перевірте брокер вручну, ще без прошивки** — у двох окремих терміналах:
```bash
mosquitto_sub -h <ВАША_IP> -t "logger/telemetry"
```
```bash
mosquitto_pub -h <ВАША_IP> -t "logger/telemetry" -m "test"
```
Якщо в першому терміналі з'явився рядок `test` — брокер живий і
доступний по мережі, можна переходити до прошивки.

> **🔴 БЕЗПЕКА: `allow_anonymous true` — лише для лабораторної мережі**
> Такий конфіг приймає підключення від будь-кого в тій самій Wi-Fi мережі
> без жодного пароля — прийнятно для навчального стенда за закритим
> роутером, але неприпустимо для брокера, доступного з інтернету. Логін/
> пароль для Mosquitto — Челендж 2.
>
> **Якщо встановлювач Windows сам зареєстрував Mosquitto як службу
> Windows** ("Mosquitto Broker" у `services.msc`), яка автоматично
> стартує зі стоковим конфігом (без `9001`, часто прив'язана лише до
> `127.0.0.1`) — вона не заважає `setup_broker.sh`/ручному запуску (різні
> IP-адреси прив'язки, порти не конфліктують), але й не замінює його:
> веб-консоль і плата все одно потребують саме вашого `broker/mosquitto.conf`
> із `listener 9001`. Вимкнути зайву службу (не обов'язково) — з
> **адміністраторського** PowerShell: `Stop-Service mosquitto; Set-Service
> mosquitto -StartupType Disabled`.

### Частина 2 — esp-mqtt: підключення, publish/subscribe

**`components/mqtt_link/include/mqtt_link.h`:**
```c
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mqtt_link_led_cb_t)(bool led_on);
typedef void (*mqtt_link_time_synced_cb_t)(void);

// Викликати ПІСЛЯ wifi_prov_init_and_connect() (Частина 0) -- ця функція вже
// не займається Wi-Fi сама, лише SNTP (для Tiny RTC) + підключення до брокера.
void mqtt_link_init(mqtt_link_led_cb_t on_led_command, mqtt_link_time_synced_cb_t on_time_synced);
bool mqtt_link_is_connected(void);
void mqtt_link_publish_telemetry(const char *json_payload);

#ifdef __cplusplus
}
#endif
```

**`components/mqtt_link/mqtt_link.c`:**
```c
#include "mqtt_link.h"
#include "mqtt_client.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

#define TOPIC_TELEMETRY   "logger/telemetry"
#define TOPIC_CONTROL     "logger/control/led"   // payload: "ON" / "OFF"

static const char *TAG = "MQTT_LINK";
static esp_mqtt_client_handle_t s_client;
static bool s_connected = false;
static mqtt_link_led_cb_t s_led_cb = NULL;

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;
    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "MQTT connected to %s, subscribing to %s", CONFIG_MQTT_BROKER_URI, TOPIC_CONTROL);
        esp_mqtt_client_subscribe(s_client, TOPIC_CONTROL, 1);   // QoS 1
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected -- esp-mqtt will retry reconnecting in the background");
        break;
    case MQTT_EVENT_DATA: {
        // event->topic/event->data НЕ null-terminated -- порівнюємо за довжиною, копіюємо в локальний буфер
        if (strncmp(event->topic, TOPIC_CONTROL, event->topic_len) == 0 &&
            strlen(TOPIC_CONTROL) == event->topic_len) {
            char payload[8] = {0};
            int len = event->data_len < 7 ? event->data_len : 7;
            memcpy(payload, event->data, len);
            bool led_on = (strcmp(payload, "ON") == 0);
            ESP_LOGI(TAG, "Control command: %s -> LED %s", payload, led_on ? "ON" : "OFF");
            if (s_led_cb != NULL) s_led_cb(led_on);
        }
        break;
    }
    default:
        break;
    }
}

void mqtt_link_init(mqtt_link_led_cb_t on_led_command, mqtt_link_time_synced_cb_t on_time_synced)
{
    s_led_cb = on_led_command;

    // Wi-Fi вже підключений (wifi_prov, Частина 0) -- синхронізуємо час один раз, щоб "засіяти" Tiny RTC (Крок 3).
    // Звичайний mqtt:// не потребує коректного часу для самого підключення -- це лише для RTC.
    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&sntp_cfg);
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) == ESP_OK && on_time_synced != NULL) {
        on_time_synced();
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URI,
        .credentials.client_id = CONFIG_MQTT_CLIENT_IDENTIFIER,
    };
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
}

bool mqtt_link_is_connected(void)
{
    return s_connected;
}

void mqtt_link_publish_telemetry(const char *json_payload)
{
    if (!s_connected) return;   // немає сенсу намагатись публікувати без з'єднання
    esp_mqtt_client_publish(s_client, TOPIC_TELEMETRY, json_payload, 0, 1, 0);   // QoS 1, retain=0
}
```

**`components/mqtt_link/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "mqtt_link.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt esp_netif lwip log freertos
)
```

**`main/Kconfig.projbuild`:**
```
menu "IoT Logger — MQTT Configuration"

    config MQTT_BROKER_URI
        string "MQTT broker URI (mqtt://IP:1883)"
        default "mqtt://192.168.1.10:1883"
        help
            IP-адреса комп'ютера з Mosquitto (Частина 1) у тій самій Wi-Fi
            мережі, куди підключився логер через provisioning (Частина 0).

    config MQTT_CLIENT_IDENTIFIER
        string "MQTT Client ID"
        default "esp32s3-iot-logger-01"

endmenu
```

> **🟣 РОЗБІР: чому саме так**
> - **`esp_mqtt_client`, а не ручний цикл підключення** — на відміну від
>   низькорівневих MQTT-бібліотек, `esp-mqtt` сам тримає фонову задачу,
>   сам перепідключається з експоненційною затримкою після
>   `MQTT_EVENT_DISCONNECTED`, і сам відповідає на `PINGREQ`/`PINGRESP` —
>   з коду прикладного рівня це не видно взагалі, і саме тому тут немає
>   ані власного `TransportInterface_t`, ані окремого циклу
>   `ProcessLoop()`, які знадобились би для нижчорівневих бібліотек.
> - **`mqtt_cfg.broker.address.uri = CONFIG_MQTT_BROKER_URI`** — рядок
>   виду `"mqtt://192.168.1.10:1883"` (без `s` в `mqtt`, порт `1883`) —
>   схема `mqtt://` означає звичайне, незашифроване TCP-з'єднання; це
>   свідомий, прийнятний вибір для приватної лабораторної мережі, а не
>   недогляд.
> - **SNTP лишається, хоч сам MQTT його не вимагає** — єдина причина, з
>   якої він тут потрібен, це "засіяти" Tiny RTC (Крок 3) реальним часом
>   одного разу після підключення до інтернету; протокол `mqtt://`
>   (на відміну від TLS-з'єднань) жодних вимог до системного часу не
>   висуває взагалі.
> - **`CONFIG_MQTT_BROKER_URI`/`CONFIG_MQTT_CLIENT_IDENTIFIER` — Kconfig, а
>   не хардкод** — той самий принцип, що й у Частині 0 для Wi-Fi:
>   IP-адреса вашого конкретного брокера й ідентифікатор пристрою
>   змінюються від збірки до збірки й не повинні жити в `.c`-файлі.

### Завдання 8.1 (обов'язкове)
Виконайте Частину 1 (встановлення й перевірка Mosquitto вручну). У
`idf.py menuconfig` → `IoT Logger — MQTT Configuration` впишіть
`mqtt://<ВАША_IP>:1883` і оберіть унікальний `Client ID`. Прошийте логер,
переконайтесь, що консоль показує `MQTT connected to ..., subscribing to
logger/control/led`.

### Завдання 8.2 (обов'язкове)
У терміналі `mosquitto_sub -h <ВАША_IP> -t "logger/telemetry"` — поки що
телеметрія ще не публікується (з'явиться в Кроці 9), тож тимчасово додайте
в `app_main` виклик `mqtt_link_publish_telemetry("{\"test\":1}")` у циклі
й переконайтесь, що рядки доходять до підписника.

### Завдання 8.3 (обов'язкове)
Виконайте `mosquitto_pub -h <ВАША_IP> -t "logger/control/led" -m "ON"`.
Переконайтесь, що консоль показує `Control command: ON -> LED ON`.

### Завдання 8.4 — запитання для обговорення
Вимкніть Mosquitto на комп'ютері (`Ctrl+C`), не чіпаючи плату. Що покаже
консоль `idf.py monitor` — одразу помилку, чи тишу? Тепер знову запустіть
брокер — скільки часу минає до появи `MQTT підключено`, і **який рядок
коду з `mqtt_link.c` за це відповідає**, якщо ви ніде явно не викликали
`esp_mqtt_client_reconnect()`?

---

## 10 · Крок 9 · Інтеграція: головний цикл логера, дисплей і LED ⏱ 35 хв

> **🟢 МЕТА**
> Об'єднати десять компонентів: `wifi_prov` дає мережу без хардкоду
> паролів, два BME280 дають показання, Tiny RTC — час, OLED — живий
> дашборд, ADC-DMA — локальний поріг, `mqtt_link` (esp-mqtt) публікує й
> одночасно приймає дистанційне керування тим самим LED-індикатором.

**`components/led_ctrl/include/led_ctrl.h`:**
```c
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void led_ctrl_init(void);
void led_ctrl_set(bool on);

#ifdef __cplusplus
}
#endif
```

**`components/led_ctrl/led_ctrl.c`:**
```c
#include "led_ctrl.h"
#include "driver/gpio.h"

#define LED_PIN   GPIO_NUM_15

void led_ctrl_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(LED_PIN, 0);   // безпечний стан за замовчуванням -- LED вимкнений
}

void led_ctrl_set(bool on)
{
    gpio_set_level(LED_PIN, on ? 1 : 0);
}
```

**`components/led_ctrl/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "led_ctrl.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
```

**`main/app_main.c` (фінальна версія):**
```c
#include "wifi_prov.h"
#include "i2c_bus.h"
#include "env_i2c.h"
#include "env_spi.h"
#include "rtc_ds1307.h"
#include "oled_display.h"
#include "ctrl_adc.h"
#include "led_ctrl.h"
#include "mqtt_link.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <time.h>

// Челендж 1: "мертва рука" для дистанційного override -- якщо з моменту
// останньої MQTT-команди минуло більше цього часу, керування LED саме
// повертається до локального порогу (ручки), а не лишається на MQTT
// назавжди (див. обговорення Завдання 9.2).
#define MQTT_OVERRIDE_TIMEOUT_US   (60LL * 1000 * 1000)

static const char *TAG = "IOT_LOGGER";
static volatile bool s_mqtt_override_on = false;
static volatile bool s_mqtt_override_active = false;
static volatile int64_t s_mqtt_override_last_cmd_us = 0;

// Викликається з mqtt_link (Крок 8) з контексту MQTT-задачі -- лише виставляє прапорці
static void on_mqtt_led_command(bool led_on)
{
    s_mqtt_override_on = led_on;
    s_mqtt_override_active = true;
    s_mqtt_override_last_cmd_us = esp_timer_get_time();
}

// Викликається з mqtt_link ОДРАЗУ після успішного SNTP (Крок 8, Частина 2) -- "засіює" Tiny RTC
static void on_time_synced(void)
{
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    rtc_ds1307_set_from_tm(&timeinfo);
}

// Один явний блок PASS/FAIL по кожному пристрою -- щоб не здогадуватись
// по розкиданих логах окремих компонентів, чи все підключено правильно.
// Якщо ми взагалі дійшли до цієї функції -- I2C-шина, BME280 (I2C) і OLED
// вже пройшли власні ESP_ERROR_CHECK у своїх *_init() вище (інакше плата
// вже перезавантажилась би, Крок 2/3); тут перевіряється решта, що не
// падає fatal сама -- RTC, живий SPI-цикл, і візуальні LED/ADC.
static void run_wiring_self_test(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " WIRING SELF-TEST");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "[OK]   I2C bus              SDA=GPIO8 SCL=GPIO9");
    ESP_LOGI(TAG, "[OK]   BME280 I2C (0x76)    init passed (see ENV_I2C log above)");
    ESP_LOGI(TAG, "[OK]   SSD1306 OLED (0x3C)  init passed -- wolf should blink on screen now");

    rtc_time_t t;
    if (rtc_ds1307_read_time(&t)) {
        ESP_LOGI(TAG, "[OK]   Tiny RTC DS1307 (0x68)  read ok: %02u:%02u:%02u", t.hour, t.min, t.sec);
    } else {
        ESP_LOGE(TAG, "[FAIL] Tiny RTC DS1307 (0x68)  no response -- check VCC/GND/SCL/SDA, battery seated");
    }

    // Правдоподібний діапазон кімнатної температури -- і для I2C, і для SPI,
    // недостатньо просто перевірити esp_err_t/bool: I2C підтверджує обрив
    // апаратним NACK (Модуль 4.3), а от SPI ACK/NACK не має взагалі -- майстер
    // "успішно" клацає тактові імпульси навіть у порожнечу, і при відключеному
    // MISO читає лише підтягнуті вгору "1" (chip_id=0xFF, температура=0.0),
    // без жодної помилки esp_err_t. Тому FAIL тут визначається за
    // правдоподібністю самого значення, а не лише за bool з *_read().
    float t_i2c, p_i2c, h_i2c;
    bool i2c_plausible = env_i2c_read(&t_i2c, &p_i2c, &h_i2c) && t_i2c > 5.0f && t_i2c < 50.0f;
    if (i2c_plausible) {
        ESP_LOGI(TAG, "[OK]   BME280 I2C (0x76)   live read: %.1fC %.0f%%RH %.0fhPa", t_i2c, h_i2c, p_i2c);
    } else {
        ESP_LOGE(TAG, "[FAIL] BME280 I2C (0x76)   read=%.1fC -- implausible/failed, check wiring", t_i2c);
    }

    float t_spi, p_spi, h_spi;
    bool spi_plausible = env_spi_read(&t_spi, &p_spi, &h_spi) && t_spi > 5.0f && t_spi < 50.0f;
    if (spi_plausible) {
        ESP_LOGI(TAG, "[OK]   BME280 SPI (CS=17)  live read: %.1fC %.0f%%RH %.0fhPa", t_spi, h_spi, p_spi);
    } else {
        ESP_LOGE(TAG, "[FAIL] BME280 SPI (CS=17)  read=%.1fC -- 0.0/implausible means SPI has NO ack, "
                      "a disconnected wire still 'succeeds' with garbage; check SCK/SDI/SDO/CS (GPIO5/6/16/17)",
                 t_spi);
    }

    ESP_LOGI(TAG, "[--]   LED indicator (GPIO15)  blinking 3x now -- watch the board");
    for (int i = 0; i < 3; i++) {
        led_ctrl_set(true);  vTaskDelay(pdMS_TO_TICKS(150));
        led_ctrl_set(false); vTaskDelay(pdMS_TO_TICKS(150));
    }

    ESP_LOGI(TAG, "[--]   Potentiometer (GPIO4)   threshold now = %.1fC -- turn the knob and re-check",
             ctrl_adc_get_threshold_c());
    ESP_LOGI(TAG, "========================================");
}

void app_main(void)
{
    led_ctrl_init();
    i2c_bus_init();          // ОДИН раз, до будь-якого з трьох I2C-споживачів нижче
    env_i2c_init();
    rtc_ds1307_init();
    oled_display_init();
    env_spi_init();
    ctrl_adc_init();

    run_wiring_self_test();

    ESP_LOGI(TAG, "Showing boot wolf animation (~5s)...");
    oled_play_wolf_boot_animation();

    wifi_prov_init_and_connect();   // Крок 8, Частина 0 -- SoftAP при першому запуску, потім одразу STA
    mqtt_link_init(on_mqtt_led_command, on_time_synced);   // брокер/client ID -- з idf.py menuconfig, Крок 8

    while (1) {
        rtc_time_t t;
        bool rtc_ok = rtc_ds1307_read_time(&t);

        float t_i2c = 0, p_i2c = 0, h_i2c = 0;
        bool i2c_ok = env_i2c_read(&t_i2c, &p_i2c, &h_i2c);

        float t_spi = 0, p_spi = 0, h_spi = 0;
        bool spi_ok = env_spi_read(&t_spi, &p_spi, &h_spi);

        float threshold_c = ctrl_adc_get_threshold_c();
        uint32_t adc_raw = ctrl_adc_get_raw();

        // Челендж 1: "мертва рука" -- override з MQTT сам звільняє керування
        // назад ручці, якщо давно не було нової команди (а не висить вічно,
        // як обговорювалось у Завданні 9.2).
        if (s_mqtt_override_active &&
            (esp_timer_get_time() - s_mqtt_override_last_cmd_us) > MQTT_OVERRIDE_TIMEOUT_US) {
            s_mqtt_override_active = false;
            ESP_LOGI(TAG, "MQTT override timed out (%lld s) -- returning LED control to the local threshold",
                     (long long)(MQTT_OVERRIDE_TIMEOUT_US / 1000000));
        }

        // --- Рішення про LED: локальний поріг АБО дистанційний override з MQTT ---
        bool local_alarm = (i2c_ok && t_i2c > threshold_c) || (spi_ok && t_spi > threshold_c);
        bool led_on = s_mqtt_override_active ? s_mqtt_override_on : local_alarm;
        led_ctrl_set(led_on);
        const char *led_source = s_mqtt_override_active ? "mqtt" : "local";

        // --- Дисплей: живий дашборд (замінює вовка одразу після Крок 9 стартує) ---
        if (rtc_ok) {
            oled_draw_dashboard(&t, t_i2c, h_i2c, p_i2c, t_spi, h_spi, p_spi,
                                 threshold_c, led_on, led_source);
        }

        // --- Збірка JSON і публікація ---
        char utc_time[16] = "--:--:--";   // [16], не [9] -- ЕРАТА п.4
        if (rtc_ok) snprintf(utc_time, sizeof(utc_time), "%02u:%02u:%02u", t.hour, t.min, t.sec);

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "utc_time", utc_time);
        cJSON_AddBoolToObject(root, "i2c_ok", i2c_ok);
        if (i2c_ok) {
            cJSON_AddNumberToObject(root, "temp_i2c_c", t_i2c);
            cJSON_AddNumberToObject(root, "hum_i2c_rh", h_i2c);
            cJSON_AddNumberToObject(root, "press_i2c_hpa", p_i2c);
        }
        cJSON_AddBoolToObject(root, "spi_ok", spi_ok);
        if (spi_ok) {
            cJSON_AddNumberToObject(root, "temp_spi_c", t_spi);
            cJSON_AddNumberToObject(root, "hum_spi_rh", h_spi);
            cJSON_AddNumberToObject(root, "press_spi_hpa", p_spi);
        }
        cJSON_AddNumberToObject(root, "threshold_c", threshold_c);
        cJSON_AddNumberToObject(root, "adc_raw", adc_raw);
        cJSON_AddBoolToObject(root, "led_on", led_on);
        cJSON_AddStringToObject(root, "led_source", led_source);

        char *json_str = cJSON_PrintUnformatted(root);
        mqtt_link_publish_telemetry(json_str);

        ESP_LOGI(TAG, "%s | I2C T=%.1fC | SPI T=%.1fC | threshold=%.1fC | LED=%s(%s) | MQTT=%s",
                 utc_time, t_i2c, t_spi, threshold_c, led_on ? "ON" : "OFF", led_source,
                 mqtt_link_is_connected() ? "up" : "down");

        cJSON_free(json_str);
        cJSON_Delete(root);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

**Оновіть `main/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES i2c_bus env_i2c env_spi rtc_ds1307 oled_display ctrl_adc led_ctrl wifi_prov mqtt_link json
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`i2c_bus_init()` — перший виклик серед I2C-компонентів** — саме тому,
>   що `env_i2c_init()`, `rtc_ds1307_init()`, `oled_display_init()`
>   лише додають пристрій на вже готову шину (Крок 1); порядок виклику
>   цих трьох `_init()` між собою не має значення, але всі три мають
>   виконатись **після** `i2c_bus_init()`.
> - **`oled_play_wolf_boot_animation()` викликається до
>   `wifi_prov_init_and_connect()`** — SoftAP-провіжинінг чи звичайне
>   STA-підключення (Крок 8, Частина 0) саме по собі може зайняти кілька
>   секунд (а при першому запуску — і хвилини, поки хтось відкриє
>   застосунок на телефоні), тож вовк на екрані одразу дає користувачу
>   видиму ознаку "пристрій живий і завантажується" ще до того, як мережа
>   взагалі підключиться.
> - **`wifi_prov_init_and_connect()` — окремий виклик до `mqtt_link_init()`,
>   а не всередині нього** — `mqtt_link` більше не знає нічого про Wi-Fi
>   (Крок 8, Частина 2): він просто припускає, що на момент виклику мережа
>   вже є, і одразу переходить до SNTP і TLS. Це та сама межа
>   відповідальності, що й між `env_i2c`/`env_spi` — кожен компонент
>   відповідає рівно за один рівень стека.
> - **`on_time_synced()` — окремий callback, симетричний до
>   `on_mqtt_led_command()`** — `mqtt_link` не має `REQUIRES rtc_ds1307`;
>   він лише повідомляє "час синхронізовано", а що саме з цим робити
>   (записати в Tiny RTC) вирішує виключно `main`.
> - **`led_ctrl_set()` — звичайний `gpio_set_level()`, без ШІМ і без
>   зовнішнього драйвера** — LED-індикатор бінарний (увімк/вимк), на
>   відміну від, наприклад, серво чи зумера з Модулів 3.x, тут не потрібна
>   ані LEDC-периферія, ані окреме джерело живлення.
> - **`run_wiring_self_test()` перевіряє *значення*, а не лише
>   `esp_err_t`/`bool` — і це принципово для SPI.** I²C апаратно
>   підтверджує обрив через NACK (Модуль 4.3) — `env_i2c_read()` чесно
>   поверне `false`. У SPI такого підтвердження немає: майстер сам генерує
>   тактовий сигнал і "успішно" читає біти, навіть якщо `MISO` ні до чого
>   не підключений — лінія просто висить у "1" від внутрішньої підтяжки,
>   і `env_spi_read()` поверне `true` з `chip_id=0xFF`/температурою рівно
>   `0.0`. Тому самотест окремо перевіряє, що прочитане значення
>   **правдоподібне** (`5°C < T < 50°C`), а не лише що виклик не впав.

**Що покаже самотест на екрані/у консолі одразу після ініціалізації**
(до вовка) — приклад реального виводу з відключеним SPI-датчиком:
```
I (752) IOT_LOGGER: ========================================
I (762) IOT_LOGGER:  WIRING SELF-TEST
I (762) IOT_LOGGER: ========================================
I (772) IOT_LOGGER: [OK]   I2C bus              SDA=GPIO8 SCL=GPIO9
I (772) IOT_LOGGER: [OK]   BME280 I2C (0x76)    init passed (see ENV_I2C log above)
I (782) IOT_LOGGER: [OK]   SSD1306 OLED (0x3C)  init passed -- wolf should blink on screen now
I (792) IOT_LOGGER: [OK]   Tiny RTC DS1307 (0x68)  read ok: 23:01:11
I (812) IOT_LOGGER: [OK]   BME280 I2C (0x76)   live read: 30.7C 70%RH 1017hPa
E (822) IOT_LOGGER: [FAIL] BME280 SPI (CS=17)  read=0.0C -- 0.0/implausible means SPI has NO ack, a
                     disconnected wire still 'succeeds' with garbage; check SCK/SDI/SDO/CS (GPIO5/6/16/17)
I (822) IOT_LOGGER: [--]   LED indicator (GPIO15)  blinking 3x now -- watch the board
I (1732) IOT_LOGGER: [--]   Potentiometer (GPIO4)   threshold now = 35.0C -- turn the knob and re-check
```
`[OK]`/`[FAIL]` — автоматично перевірено; `[--]` — потребує вашого ока
(побачили, як моргнув LED тричі? побачили, як вовк моргає на дисплеї?
покрутили ручку — число справді змінюється?).

### Завдання 9.1
Зберіть повний проєкт, прошийте. Переконайтесь, що перші ~5 секунд на
дисплеї моргає вовк, а потім екран перемикається на дашборд (час, обидва
датчики, поріг, стан LED) і оновлюється щодві секунди.

### Завдання 9.2 — запитання для обговорення
Крутіть ручку потенціометра так, щоб поріг опустився нижче поточної
кімнатної температури — LED повинен увімкнутись, і напис на дисплеї
(`led_source`) повинен показати `local`. Опублікуйте `"OFF"` у
`logger/control/led` — що станеться з написом на дисплеї, і чому він
більше **не** реагує на подальші зміни ручки, аж доки плату не
перезавантажать (підказка: подумайте про третій стан і про Челендж 1)?

---

## 11 · Крок 10 · Живий дебаг через вбудований USB-Serial-JTAG ⏱ 25 хв

> **🟢 МЕТА**
> Застосувати Модуль 4.7 не як окрему теорію, а як інструмент діагностики
> вже зібраного логера — впіймати систему "на гарячому" в момент
> формування JSON, використовуючи той самий USB-кабель, яким і так тече
> `idf.py monitor`.

### Крок 10.1 — підключення дебагера
У `Espressif-IDE`: `Run` → `Debug Configurations` → `GDB OpenOCD
Debugging`, `.cfg`-файл `board/esp32s3-builtin.cfg` (вбудований
`USB-Serial-JTAG` — Модуль 4.7, Частина 2). Жодного окремого `ESP-PROG` не
потрібно.

### Крок 10.2 — breakpoint у момент публікації
Поставте breakpoint на рядку `mqtt_link_publish_telemetry(json_str);` у
`main/app_main.c`. Запустіть `Debug`. Дочекайтесь, поки логер пройде повне
коло (RTC→I²C→SPI→ADC→дисплей) і зупиниться рівно перед публікацією.

### Крок 10.3 — інспекція значень наживо
У панелі `Variables` розгорніть `t`, `t_i2c`, `t_spi`, `threshold_c`,
`led_on`, `json_str` — побачите **реальні** значення поточного циклу, ті
самі, що щойно намальовані на OLED.

### Крок 10.4 — SMP debug на двох ядрах
У панелі `Debug` перевірте, що обидва потоки (`Core 0`, `Core 1`) позначені
зупиненими одночасно (Модуль 4.7, Частина 2), попри те, що
`adc_read_task`, MQTT-задача і головний цикл могли в момент зупинки
виконуватись на різних ядрах.

### Крок 10.5 — одночасний UART-лог
Не закриваючи debug-сесію, відкрийте паралельно `Serial Monitor` — Модуль
4.7, Частина 3. `Resume` — монітор одразу відновлює потік логів.

### Завдання 10.1 (обов'язкове)
Виконайте Кроки 10.1-10.5 повністю; зробіть текстовий запис значень
`t_i2c`/`t_spi` з панелі `Variables` в один і той самий момент зупинки.

### Завдання 10.2 — запитання для обговорення
Якби `mqtt_link_publish_telemetry()` зависла саме в момент, коли ви
поставили на неї breakpoint і виконуєте `Resume`, як SMP debug допоміг би
відрізнити "завис лише мережевий стек" від "зависла вся система" —
включно з тим, чи продовжує оновлюватись дисплей?

---

## 12 · Крок 11 · Просте тестування ⏱ 15 хв

На платі — два BME280, Tiny RTC, OLED-дисплей, ручка порогу, LED-індикатор
тривоги, MQTT-канал до власного брокера Mosquitto.

**Wi-Fi provisioning**
- Перший запуск (порожня NVS) → у списку Wi-Fi телефону з'являється
  `PROV_iot_logger_XXXX`; після передачі SSID/пароля через застосунок
  ESP SoftAP Provisioning пристрій підключається як STA.
- Будь-який наступний запуск → SoftAP не з'являється, пристрій одразу йде
  в STA з раніше збереженими даними (NVS).

**Дисплей**
- Перші ~5 секунд після живлення — моргає вовк.
- Далі — дашборд: час, `I2C ...`, `SPI ...`, поріг і стан LED, оновлення
  кожні 2 секунди.

**Два BME280 (I²C проти SPI)**
- Обидва `i2c_ok`/`spi_ok` — `true`, значення на дисплеї й у JSON близькі.

**Tiny RTC**
- Час на дисплеї монотонно зростає навіть без Wi-Fi (до першого
  SNTP-синку може бути неправильним — це очікувано, поки RTC ще не
  "засіяний"; після Кроку 8 стає коректним і лишається таким навіть після
  перезавантаження плати, завдяки батарейці CR2032).

**Ручка потенціометра (локальний поріг)**
- Крути нижче поточної температури → LED вмикається, `led_source:local`.
- Крути вище → LED вимикається.

**MQTT-брокер (дистанційне керування)**
- `mosquitto_pub -h <ВАША_IP> -t "logger/control/led" -m "ON"`/`"OFF"` →
  LED реагує незалежно від ручки, `led_source:mqtt`.
- `mosquitto_sub -h <ВАША_IP> -t "logger/telemetry"` → щодві секунди новий
  JSON-рядок із показаннями обох датчиків.

**Обрив I²C (діагностика)**
- Тимчасово від'єднайте `SDA` → `env_i2c`, RTC і дисплей одночасно
  перестають отримувати нові дані (спільна шина, Крок 3-4); SPI-датчик і
  MQTT-зв'язок продовжують працювати незалежно.

**Підсумок одним реченням:** одна й та сама температура приміщення приходить
до логера двома незалежними шляхами (I²C і SPI), апаратний годинник
тримає час незалежно від мережі, а LED-індикатор слухає два незалежні
джерела команди (локальна ручка й MQTT-брокер) — надійність через
незалежні, паралельні шляхи до одного результату, видима прямо на екрані
пристрою.

### 12.1 · Практична діагностика: як читати серійний лог при збоях

Це — реальні симптоми, з якими стикались під час бринг-апу цього самого
проєкту (не гіпотетичні), разом із тим, як їх однозначно підтвердити по
логу `idf.py monitor` (або будь-якому serial-термінал на 115200 8N1),
а не гадати.

**Симптом А: плата в нескінченному циклі перезавантажень одразу після
живлення.** У логу — знайомий блок:
```
ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE) at 0x...
file: "./components/env_i2c/env_i2c.c" line 36
func: env_i2c_init
expression: wtr(0xD0, &chip_id, 1)

abort() was called at PC 0x... on core 0
...
Rebooting...
```
`rst:0xc (RTC_SW_CPU_RST)` на наступному циклі підтверджує: це не
"зависання", а свідомий `abort()` через `ESP_ERROR_CHECK` (Крок 2 навмисно
fail-fast — будь-яка I²C/SPI-помилка при ініціалізації валить прошивку,
а не тихо продовжує з непрочитаним датчиком). Рядки `file:`/`func:`/
`expression:` над `abort()` показують **точно**, який виклик провалився
— тут це читання Chip ID BME280 №1 по I²C. Найчастіша причина —
апаратна, не в коді: датчик фізично не підключений/не живиться, переплутані
`SDA`/`SCL`, або поганий контакт на макетній платі (Крок 0).
`ESP_ERR_INVALID_STATE` саме на найпершій I²C-транзакції типово означає,
що шина взагалі не може завершити `START` — лінії "не ворушаться" так,
як мали б за справної проводки.

**Симптом Б: на дисплеї лише вовк, а далі нічого (~назавжди).** Це
**не баг** — перевірте лог, там має бути:
```
I (...) WIFI_PROV: Not provisioned -- starting SoftAP "PROV_iot_logger_XXXX"
I (...) wifi_prov_mgr: Provisioning started with service name : PROV_iot_logger_XXXX
I (...) WIFI_PROV: Waiting for IP...
```
`wifi_prov_init_and_connect()` (Крок 8, викликається одразу після вовка
в Кроці 9) **блокує** увесь подальший код, доки плата не отримає IP —
а IP не буде, доки хтось не пройде провіжинінг через телефон (Завдання
8.0.1). Дашборд і MQTT просто ще не настали в послідовності виконання
`app_main()`.

**Симптом В: точка доступу `PROV_iot_logger_XXXX` була, а тепер зникла
й більше не з'являється — хоча Wi-Fi так і не підключився.** У логу:
```
I (...) WIFI_PROV: Already provisioned -- connecting as a regular STA
I (...) WIFI_PROV: Waiting for IP with stored credentials (timeout 300 s)...
```
а далі — тиша (`WIFI_EVENT_STA_DISCONNECTED` в обробнику Кроку 8 сам
мовчки перевикликає `esp_wifi_connect()`, без логів на кожну спробу). Це
означає: в NVS уже **збережені** SSID/пароль з попередньої спроби
провіжинінгу, і саме вони зараз не працюють (застаріла мережа, помилка
при вводі пароля, роутер вимкнений). Завдяки таймауту з ЕРАТА п.7 плата
сама почекає `STA_CONNECT_TIMEOUT_MS` і, якщо `WIFI_PROV: Stored
Wi-Fi credentials did not connect in time...` таки з'явиться в логу —
скине провіжинінг і сама перезавантажиться назад у SoftAP: просто
почекайте (з поточним значенням — до 5 хв) і пройдіть провіжинінг
(Завдання 8.0.1) ще раз з правильними даними.

> **🟠 На практиці навіть 60 секунд виявилось замало.** У реальному
> тестуванні цього проєкту (не гіпотетично) `WIFI_PROV: Stored Wi-Fi
> credentials did not connect in time...` з'являвся стабільно й при
> 60-секундному таймауті — облікові дані були правильні (провіжинінг
> одного разу вже успішно спрацював раніше), але сам STA-`esp_wifi_connect()`
> послідовно не встигав завершитись. Це вже **не питання пам'яті чи
> коду проєкту** — NVS зберігає SSID/пароль коректно щоразу (перевірено
> логом); причина повільного/невдалого STA — на рівні самої Wi-Fi-мережі
> (роутер тимчасово "притримує" клієнта, що часто перепідключається з
> тим самим MAC; мережа 5GHz-only чи WPA3-only, які ESP32-S3 не
> підтримує; слабкий сигнал у місці макетної плати). Тимчасово підняли
> `STA_CONNECT_TIMEOUT_MS` до `300000` (5 хв) — з великим запасом, поки
> причина повільного STA не з'ясована окремо (перевірте налаштування
> роутера: діапазон 2.4GHz увімкнено, тип шифрування — не WPA3-only).

Ручний спосіб (якщо потрібно скинути негайно, не
чекаючи таймаут) — стерти лише розділ `nvs` через `esptool`, без повторного
перепрошивання застосунку:
```bash
python -m esptool --chip esp32s3 -p COMx erase_region 0x9000 0x6000
```
(офсет `0x9000` і розмір `0x6000` розділу `nvs` беруться з таблиці
розділів на самому початку лога завантаження, рядок `boot: Partition
Table`).

**Симптом Г: плата підключилась до Wi-Fi, `MQTT connected to ...` є в
логу, дашборд на екрані оновлюється — а веб-сторінка (Челендж 7) у
браузері показує `OFFLINE`.** Це вже не про плату, а про сам брокер/
сторінку — дивіться діагностику в кінці Челенджу 7 (websockets-listener
Mosquitto, mixed content при відкритті сторінки по `https://`).

---

## 13 · Челенджі ⏱ ~1.5 год

### Челендж 1 · Таймаут MQTT-override ⏱ 15 хв (реалізовано нижче)
Виправте поведінку із Завдання 9.2: додайте `int64_t` мітку часу останньої
MQTT-команди (`esp_timer_get_time()`) і скидайте `s_mqtt_override_active`
у `false`, якщо з моменту останньої команди минуло понад 60 секунд —
"мертва рука" для мережевого керування.

**Готове рішення.** У `main/app_main.c` (Крок 9) додається:
```c
#include "esp_timer.h"

// Челендж 1: "мертва рука" для дистанційного override -- якщо з моменту
// останньої MQTT-команди минуло більше цього часу, керування LED саме
// повертається до локального порогу (ручки), а не лишається на MQTT
// назавжди (див. обговорення Завдання 9.2).
#define MQTT_OVERRIDE_TIMEOUT_US   (60LL * 1000 * 1000)

static volatile int64_t s_mqtt_override_last_cmd_us = 0;

static void on_mqtt_led_command(bool led_on)
{
    s_mqtt_override_on = led_on;
    s_mqtt_override_active = true;
    s_mqtt_override_last_cmd_us = esp_timer_get_time();   // <-- нове
}
```
і в самому циклі, перед обчисленням `led_on` (Крок 9):
```c
if (s_mqtt_override_active &&
    (esp_timer_get_time() - s_mqtt_override_last_cmd_us) > MQTT_OVERRIDE_TIMEOUT_US) {
    s_mqtt_override_active = false;
    ESP_LOGI(TAG, "MQTT override timed out (%lld s) -- returning LED control to the local threshold",
             (long long)(MQTT_OVERRIDE_TIMEOUT_US / 1000000));
}
```
Повний код — у Кроці 9 (вище за текстом), він уже включає цей таймаут.

### Челендж 2 · Логін/пароль для Mosquitto ⏱ 15 хв
Замініть `allow_anonymous true` (Крок 8, Частина 1) на автентифікацію:
`mosquitto_passwd -c passwd.txt esp32logger`, у `mosquitto.conf` додайте
`password_file passwd.txt` і `allow_anonymous false`. У Kconfig (Крок 8,
Частина 2) додайте `MQTT_USERNAME`/`MQTT_PASSWORD`, передайте їх через
`mqtt_cfg.credentials.username`/`.authentication.password` в
`esp_mqtt_client_config_t`. Перевірте, що `mosquitto_sub`/`mosquitto_pub`
без `-u`/`-P` більше не працюють, а прошивка — працює.

### Челендж 3 · Last Will and Testament — "останнє слово" пристрою ⏱ 15 хв
Додайте в `esp_mqtt_client_config_t` (Крок 8, Частина 2) поле
`.session.last_will` — тема `logger/status`, payload `{"online":false}`,
`qos = 1` — і опублікуйте `{"online":true}` (`retain=1`) одразу після
`MQTT_EVENT_CONNECTED`. Різко відключіть живлення плати — підписник на
`logger/status` повинен побачити `online:false`, хоча жоден рядок коду на
пристрої це явно не надсилав: сам брокер публікує will-повідомлення,
щойно TCP-з'єднання обривається без штатного `DISCONNECT`.

### Челендж 4 · Постійна анімація секунд на дисплеї ⏱ 15 хв
Замініть статичний дашборд на живішу версію: домалюйте тонку смугу
прогресу секунд (0-59) під часом через `u8g2_DrawBox()` — та сама ідея,
що згадана в `u8g2_clock_and_wolf_logo_EXAMPLE.md` (перевикористання
`ssd1306_draw_seconds_bar()` з Модуля 4.3, лише через `u8g2` API замість
ручних I²C-транзакцій).

### Челендж 5 · Retained-повідомлення для стану LED ⏱ 15 хв
Опублікуйте окремий топік `logger/status/led` з прапорцем `retain=1`
(останній аргумент `esp_mqtt_client_publish()`) щоразу, коли змінюється
`led_on`. Підключіть новий підписник (`mosquitto_sub`) **вже після** зміни
стану — переконайтесь, що він одразу отримує останнє відоме значення від
брокера, не чекаючи наступного циклу публікації логера. Поясніть, чим
`retain` відрізняється від звичайної публікації.

### Челендж 6 · Порівняння затримки I²C проти SPI на цьому проєкті ⏱ 15 хв
За зразком Модуля 4.4 (Частина 4) виміряйте `esp_timer_get_time()` навколо
повного циклу `env_i2c_read()` і навколо `env_spi_read()` окремо. Яка шина
швидша на цьому обладнанні, і чи узгоджується різниця з теоретичним
співвідношенням тактових частот двох шин (Модуль 4.4, Додаток А)?

### Челендж 7 · Веб-консоль телеметрії (MQTT over WebSocket, без хмари) ⏱ 20 хв
Той самий брокер Mosquitto з Кроку 8 вміє віддавати MQTT ще й по
WebSocket — досить одного додаткового `listener` у `mosquitto.conf`,
без жодного окремого бекенда чи хмарного сервісу:
```
listener 1883
allow_anonymous true

listener 9001
protocol websockets
```
Побудуйте одну статичну HTML-сторінку (жодного білд-кроку, жодних
npm-залежностей), яка підключається напряму з браузера до
`ws://<ВАША_IP>:9001` і:
- підписується на `logger/telemetry` (Крок 9) і в реальному часі
  показує обидва `env_i2c_read()`/`env_spi_read()` показання, поточний
  `threshold_c` і стан `led_on`/`led_source`;
- публікує `"ON"`/`"OFF"` у `logger/control/led` (той самий топік, що й
  `mosquitto_pub` у Завданні 8.3) — тобто керує тим самим LED, що й
  ручка потенціометра, лише з іншого "клієнта" MQTT.

**Ключове технічне обмеження — не сам MQTT, а протокол сторінки.**
Браузер не вміє відкривати сирий TCP (порт `1883`), лише WebSocket
(порт `9001`) — тому потрібен окремий `listener` вище. Якщо ж сама
сторінка відкрита по `https://` (наприклад, зі спільного хостингу), а
брокер віддає лише незашифрований `ws://` (без TLS, як і решта цього
проєкту — Крок 8), браузер заблокує з'єднання як **mixed content**;
рішення — відкривати сторінку локально (`file://`) чи з того самого
LAN, а не з хмарного https-хостингу, доки брокер працює без TLS.

`fetch`/`XMLHttpRequest` тут не підійдуть — потрібен саме
MQTT-протокол поверх `WebSocket`, тому або підключіть готову бібліотеку
(`mqtt.js`), або, якщо сторінка має лишатись без жодної зовнішньої
залежності, напишіть мінімальний MQTT 3.1.1-клієнт власноруч: лише
`CONNECT`/`CONNACK`, `SUBSCRIBE`/`SUBACK`, вхідний `PUBLISH` (QoS 0
достатньо — брокер сам знижує `QoS` вихідних публікацій до `QoS`
підписки), `PUBLISH` для власних команд і `PINGREQ` раз на кілька
десятків секунд, щоб з'єднання не відвалилось по `keep-alive`.

**Немає жодного HTTP API.** Ані плата, ані ця сторінка не піднімають
HTTP-сервер і не роблять `fetch()`/`XMLHttpRequest` одне до одного.
Обидва — рівноправні MQTT-клієнти **одного й того самого** брокера
Mosquitto (Крок 8): плата публікує в `logger/telemetry`/слухає
`logger/control/led`, сторінка робить те саме, лише через `WebSocket`
замість сирого TCP:
```
ESP32 (mqtt_link.c)    ──MQTT, TCP:1883──▶  Mosquitto (брокер)
Браузер (dashboard.js) ──MQTT-over-WS, TCP:9001──▶  Mosquitto (той самий брокер)
```
Брокер сам пересилає повідомлення від видавця підписникам — так само,
як і в Кроці 8 з `mosquitto_sub`/`mosquitto_pub`.

**Готове рішення (референс) — три окремі файли, не один.** На відміну
від типового HTML-прототипу "в один файл", тут розмітка/стилі/скрипт
свідомо розведені по каталогах — так простіше окремо міняти дизайн
(`css/`), не чіпаючи MQTT-клієнт (`js/`):
```
web/
└── templates/
    ├── css/
    │   └── dashboard.css      -- лише стилі (мілітарі-дизайн: темне тло,
    │                              кутові рамки, кольори хакі/бронза)
    ├── js/
    │   └── dashboard.js       -- лише логіка: мінімальний MQTT 3.1.1-клієнт
    │                              поверх WebSocket, без жодної зовнішньої
    │                              бібліотеки, і оновлення DOM
    └── html/
        └── dashboard.html     -- лише розмітка + <link>/<script src="">
                                   на два файли вище
```
Відкривати саме `html/dashboard.html` (подвійним кліком) — відносні
шляхи `../css/dashboard.css` і `../js/dashboard.js` спрацюють, лише
якщо структура каталогів збережена такою, як є; жодного білд-кроку,
сервера чи інтернету не потрібно.

**`web/templates/css/dashboard.css`:**
```css
  :root{
    --bg:#0d0f0a; --panel:#14170f; --panel-2:#191d13; --panel-3:#0f110c;
    --line:#2b3122; --line-strong:#3c4530;
    --ink:#dcdccb; --ink-dim:#8d9277; --ink-faint:#5c6350;
    --accent:#c98a3e; --accent-ink:#1a1206;
    --good:#5f9e5a; --warn:#d9a63e; --critical:#c1443c;
    --shadow: rgba(0,0,0,.45);
    --mono: ui-monospace, "Cascadia Mono", "SF Mono", Consolas, "Roboto Mono", monospace;
    --sans: Arial, "Helvetica Neue", sans-serif;
  }
  @media (prefers-color-scheme: light){
    :root{
      --bg:#eae5d6; --panel:#f3efe2; --panel-2:#ece6d4; --panel-3:#e2dcc7;
      --line:#c7bfa2; --line-strong:#a89c78;
      --ink:#211f16; --ink-dim:#5c5640; --ink-faint:#8a8264;
      --accent:#8a531b; --accent-ink:#fbf3e4;
      --good:#3f7a3a; --warn:#8f611a; --critical:#a13228;
      --shadow: rgba(60,50,20,.18);
    }
  }
  :root[data-theme="dark"]{
    --bg:#0d0f0a; --panel:#14170f; --panel-2:#191d13; --panel-3:#0f110c;
    --line:#2b3122; --line-strong:#3c4530;
    --ink:#dcdccb; --ink-dim:#8d9277; --ink-faint:#5c6350;
    --accent:#c98a3e; --accent-ink:#1a1206;
    --good:#5f9e5a; --warn:#d9a63e; --critical:#c1443c;
    --shadow: rgba(0,0,0,.45);
  }
  :root[data-theme="light"]{
    --bg:#eae5d6; --panel:#f3efe2; --panel-2:#ece6d4; --panel-3:#e2dcc7;
    --line:#c7bfa2; --line-strong:#a89c78;
    --ink:#211f16; --ink-dim:#5c5640; --ink-faint:#8a8264;
    --accent:#8a531b; --accent-ink:#fbf3e4;
    --good:#3f7a3a; --warn:#8f611a; --critical:#a13228;
    --shadow: rgba(60,50,20,.18);
  }

  *{box-sizing:border-box;}
  html,body{margin:0;padding:0;}
  body{
    background:
      radial-gradient(1200px 800px at 15% -10%, color-mix(in srgb, var(--accent) 6%, transparent), transparent 60%),
      var(--bg);
    color:var(--ink);
    font-family:var(--mono);
    padding:20px;
    min-height:100vh;
  }
  .label{
    font-family:var(--sans);
    font-stretch:condensed;
    font-weight:700;
    text-transform:uppercase;
    letter-spacing:.14em;
    font-size:11px;
    color:var(--ink-faint);
  }
  ::selection{ background:var(--accent); color:var(--accent-ink); }
  :focus-visible{ outline:2px solid var(--accent); outline-offset:2px; }

  .frame{
    max-width:1040px;
    margin:0 auto;
    border:1px solid var(--line-strong);
    background:var(--panel);
    box-shadow:0 20px 60px var(--shadow);
    position:relative;
  }
  .frame::before,.frame::after{
    content:"";
    position:absolute; width:22px; height:22px;
    pointer-events:none;
  }
  .frame::before{ top:-1px; left:-1px; border-top:2px solid var(--accent); border-left:2px solid var(--accent); }
  .frame::after{ bottom:-1px; right:-1px; border-bottom:2px solid var(--accent); border-right:2px solid var(--accent); }

  header.topbar{
    display:flex; align-items:center; justify-content:space-between;
    gap:16px; padding:20px 24px; border-bottom:1px solid var(--line);
    flex-wrap:wrap;
  }
  .unit-id{ display:flex; flex-direction:column; gap:4px; }
  .unit-id .name{
    font-family:var(--sans); font-stretch:condensed; font-weight:800;
    text-transform:uppercase; letter-spacing:.06em; font-size:22px; color:var(--ink);
    text-wrap:balance;
  }
  .unit-id .sub{ font-size:11px; letter-spacing:.14em; text-transform:uppercase; color:var(--ink-faint); }

  .topbar-right{ display:flex; align-items:center; gap:18px; }
  .clock{ font-size:20px; font-variant-numeric:tabular-nums; letter-spacing:.04em; color:var(--ink); }
  .status-chip{
    display:flex; align-items:center; gap:8px;
    border:1px solid var(--line-strong); padding:6px 12px;
    font-size:11px; letter-spacing:.12em; text-transform:uppercase;
  }
  .dot{ width:8px; height:8px; border-radius:50%; background:var(--ink-faint); flex:none; }
  .status-chip[data-state="offline"] .dot{ background:var(--ink-faint); }
  .status-chip[data-state="connecting"] .dot{ background:var(--warn); animation:pulse 1.1s ease-in-out infinite; }
  .status-chip[data-state="linked"] .dot{ background:var(--good); }
  @keyframes pulse{ 0%,100%{opacity:1;} 50%{opacity:.35;} }
  @media (prefers-reduced-motion: reduce){ .dot{ animation:none !important; } }

  .theme-toggle{
    border:1px solid var(--line-strong); background:transparent; color:var(--ink-dim);
    font-family:var(--mono); font-size:11px; letter-spacing:.1em; text-transform:uppercase;
    padding:6px 10px; cursor:pointer;
  }
  .theme-toggle:hover{ color:var(--ink); border-color:var(--accent); }

  .linkbar{
    display:flex; flex-wrap:wrap; gap:10px 16px; align-items:end;
    padding:16px 24px; border-bottom:1px solid var(--line); background:var(--panel-3);
  }
  .field{ display:flex; flex-direction:column; gap:6px; flex:1 1 200px; }
  .field input[type="text"]{
    background:var(--panel); border:1px solid var(--line-strong); color:var(--ink);
    font-family:var(--mono); font-size:13px; padding:8px 10px;
  }
  .field input[type="text"]:focus{ border-color:var(--accent); }
  .chk{ display:flex; align-items:center; gap:8px; font-size:11px; letter-spacing:.08em; text-transform:uppercase; color:var(--ink-dim); }

  .btn{
    font-family:var(--sans); font-stretch:condensed; font-weight:700;
    text-transform:uppercase; letter-spacing:.1em; font-size:12px;
    border:1px solid var(--line-strong); background:var(--panel-2); color:var(--ink);
    padding:10px 18px; cursor:pointer;
    clip-path:polygon(8px 0,100% 0,100% calc(100% - 8px),calc(100% - 8px) 100%,0 100%,0 8px);
  }
  .btn:hover{ border-color:var(--accent); color:var(--accent); }
  .btn:active{ transform:translateY(1px); }
  .btn.primary{ background:var(--accent); color:var(--accent-ink); border-color:var(--accent); }
  .btn.primary:hover{ filter:brightness(1.08); color:var(--accent-ink); }
  .btn.danger:hover{ border-color:var(--critical); color:var(--critical); }
  .btn:disabled{ opacity:.4; cursor:not-allowed; }

  main.grid{
    display:grid; grid-template-columns:1fr 1fr 1.1fr; gap:1px;
    background:var(--line);
  }
  @media (max-width:820px){ main.grid{ grid-template-columns:1fr; } }

  .tile{
    background:var(--panel); padding:20px 22px 22px;
    display:flex; flex-direction:column; gap:14px;
    position:relative;
  }
  .tile-head{ display:flex; align-items:center; justify-content:space-between; }
  .tile-head .label{ color:var(--ink-dim); }
  .pill{
    font-size:10px; letter-spacing:.12em; text-transform:uppercase;
    padding:3px 8px; border:1px solid var(--line-strong); color:var(--ink-faint);
  }
  .pill[data-ok="true"]{ color:var(--good); border-color:var(--good); }
  .pill[data-ok="false"]{ color:var(--critical); border-color:var(--critical); }
  .pill[data-ok="stale"]{ color:var(--warn); border-color:var(--warn); }

  .big-num{
    font-size:44px; line-height:1; font-variant-numeric:tabular-nums;
    color:var(--ink); letter-spacing:.01em;
  }
  .big-num .unit{ font-size:18px; color:var(--ink-dim); margin-left:4px; }
  .tile.fault .big-num{ color:var(--ink-faint); }

  .rows{ display:flex; flex-direction:column; gap:8px; margin-top:2px; }
  .row{ display:flex; align-items:baseline; justify-content:space-between; font-size:13px; }
  .row .k{ color:var(--ink-faint); letter-spacing:.06em; text-transform:uppercase; font-size:10px; }
  .row .v{ font-variant-numeric:tabular-nums; color:var(--ink); }

  .gauge-track{
    position:relative; height:8px; background:var(--panel-3); border:1px solid var(--line-strong);
  }
  .gauge-fill{ position:absolute; inset:0; width:0%; background:var(--accent); }
  .gauge-scale{ display:flex; justify-content:space-between; font-size:10px; color:var(--ink-faint); letter-spacing:.06em; }

  .led-row{ display:flex; align-items:center; gap:12px; }
  .led-lamp{
    width:16px; height:16px; border-radius:50%; border:1px solid var(--line-strong);
    background:var(--panel-3); flex:none;
  }
  .led-lamp[data-on="true"]{ background:var(--critical); box-shadow:0 0 10px 2px color-mix(in srgb, var(--critical) 55%, transparent); border-color:var(--critical); }
  .led-state{ font-family:var(--sans); font-stretch:condensed; font-weight:800; letter-spacing:.08em; font-size:16px; text-transform:uppercase; }
  .led-source{ font-size:10px; letter-spacing:.12em; text-transform:uppercase; color:var(--ink-faint); border:1px solid var(--line-strong); padding:2px 7px; }

  .btn-row{ display:flex; gap:10px; margin-top:2px; }
  .btn-row .btn{ flex:1; }

  section.feed{
    border-top:1px solid var(--line);
    background:var(--panel-3);
    padding:14px 24px 18px;
  }
  .feed-head{ display:flex; justify-content:space-between; align-items:center; margin-bottom:10px; }
  .feed-log{
    height:150px; overflow-y:auto; overflow-x:auto;
    font-size:11.5px; line-height:1.6; color:var(--ink-dim);
    border:1px solid var(--line); background:var(--panel); padding:10px 12px;
    white-space:pre;
  }
  .feed-log .t{ color:var(--ink-faint); }
  .feed-log .empty{ color:var(--ink-faint); font-style:normal; }

  footer.notes{
    border-top:1px solid var(--line); padding:14px 24px 20px;
    display:flex; flex-direction:column; gap:8px;
  }
  footer.notes p{ margin:0; font-size:11.5px; color:var(--ink-faint); line-height:1.7; }
  footer.notes code{
    font-family:var(--mono); background:var(--panel-3); border:1px solid var(--line);
    padding:1px 6px; color:var(--ink-dim);
  }
```

**`web/templates/html/dashboard.html`:**
```html
<!doctype html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>IOT-LOGGER // UNIT-01 — Data Link Console</title>
<link rel="stylesheet" href="../css/dashboard.css">
</head>
<body>

<div class="frame">
  <header class="topbar">
    <div class="unit-id">
      <div class="name">IOT-LOGGER // UNIT-01</div>
      <div class="sub">Data Logging &amp; Control Station — I²C / SPI / MQTT</div>
    </div>
    <div class="topbar-right">
      <div class="clock" id="clock">--:--:--</div>
      <div class="status-chip" id="statusChip" data-state="offline">
        <span class="dot"></span>
        <span id="statusText">OFFLINE</span>
      </div>
      <button class="theme-toggle" id="themeToggle" type="button" aria-label="Toggle light/dark theme">THEME</button>
    </div>
  </header>

  <section class="linkbar">
    <div class="field">
      <span class="label">Broker (WebSocket)</span>
      <input type="text" id="brokerUrl" value="ws://192.168.1.10:9001" autocomplete="off" spellcheck="false">
    </div>
    <div class="field" style="flex:0 1 200px;">
      <span class="label">Client ID</span>
      <input type="text" id="clientId" autocomplete="off" spellcheck="false">
    </div>
    <label class="chk"><input type="checkbox" id="autoReconnect" checked> Auto-relink</label>
    <button class="btn primary" id="linkBtn" type="button">LINK</button>
  </section>

  <main class="grid">
    <article class="tile" id="tileI2C" data-ch="i2c">
      <div class="tile-head">
        <span class="label">Sensor · I²C · BME280 0x76</span>
        <span class="pill" id="pillI2C" data-ok="false">NO DATA</span>
      </div>
      <div class="big-num"><span id="tempI2C">--</span><span class="unit">°C</span></div>
      <div class="rows">
        <div class="row"><span class="k">Humidity</span><span class="v" id="humI2C">-- %RH</span></div>
        <div class="row"><span class="k">Pressure</span><span class="v" id="presI2C">-- hPa</span></div>
      </div>
    </article>

    <article class="tile" id="tileSPI" data-ch="spi">
      <div class="tile-head">
        <span class="label">Sensor · SPI · BME280 CS5</span>
        <span class="pill" id="pillSPI" data-ok="false">NO DATA</span>
      </div>
      <div class="big-num"><span id="tempSPI">--</span><span class="unit">°C</span></div>
      <div class="rows">
        <div class="row"><span class="k">Humidity</span><span class="v" id="humSPI">-- %RH</span></div>
        <div class="row"><span class="k">Pressure</span><span class="v" id="presSPI">-- hPa</span></div>
      </div>
    </article>

    <article class="tile">
      <div class="tile-head">
        <span class="label">Threshold &amp; Alarm</span>
        <span class="pill" id="pillThresh" data-ok="false">--°C</span>
      </div>

      <div>
        <div class="gauge-track"><div class="gauge-fill" id="gaugeFill"></div></div>
        <div class="gauge-scale"><span>15°C</span><span>ADC1 · GPIO4 · DMA</span><span>35°C</span></div>
      </div>

      <div class="rows">
        <div class="row"><span class="k">Potentiometer (raw)</span><span class="v" id="adcRaw">-- / 4095</span></div>
      </div>

      <div class="led-row">
        <span class="led-lamp" id="ledLamp" data-on="false"></span>
        <span class="led-state" id="ledState">OFF</span>
        <span class="led-source" id="ledSource">--</span>
      </div>

      <div class="btn-row">
        <button class="btn" id="ledOnBtn" type="button">ARM (ON)</button>
        <button class="btn danger" id="ledOffBtn" type="button">SAFE (OFF)</button>
      </div>
    </article>
  </main>

  <section class="feed">
    <div class="feed-head">
      <span class="label">Data Link · logger/telemetry</span>
      <span class="label" id="feedCount">0 MSG</span>
    </div>
    <div class="feed-log" id="feedLog"><span class="empty">Awaiting link...</span></div>
  </section>

  <footer class="notes">
    <p><strong style="color:var(--ink-dim)">Field notes.</strong> This console speaks MQTT over WebSocket directly from the browser — no external library, no server. Point it at the same broker as the firmware's <code>CONFIG_MQTT_BROKER_URI</code>, but on the <strong>websockets</strong> listener, not the raw TCP one.</p>
    <p>Mosquitto needs a second listener alongside the existing one — add to <code>mosquitto.conf</code>: <code>listener 9001</code> / <code>protocol websockets</code>, then restart the broker.</p>
    <p>If "LINK" refuses to connect from a page opened over <code>https://</code>, that's the browser blocking insecure <code>ws://</code> from a secure page (mixed content) — save this file and open it locally (<code>file://</code>), or serve it from your own LAN, to reach the broker directly.</p>
  </footer>
</div>

<script src="../js/dashboard.js"></script>
</body>
</html>
```

**`web/templates/js/dashboard.js`:**
```js
(function(){
  "use strict";

  // ---------- theme toggle ----------
  var root = document.documentElement;
  var themeBtn = document.getElementById('themeToggle');
  themeBtn.addEventListener('click', function(){
    var cur = root.getAttribute('data-theme');
    var prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    var next;
    if (!cur) next = prefersDark ? 'light' : 'dark';
    else next = cur === 'dark' ? 'light' : 'dark';
    root.setAttribute('data-theme', next);
  });

  // ---------- clock (from telemetry, ticks locally between messages) ----------
  var clockEl = document.getElementById('clock');
  var lastUtc = null, lastUtcAt = 0;
  function tickClock(){
    if (!lastUtc) return;
    var parts = lastUtc.split(':').map(Number);
    if (parts.length !== 3 || parts.some(isNaN)) { clockEl.textContent = lastUtc; return; }
    var elapsed = Math.floor((Date.now() - lastUtcAt) / 1000);
    var total = parts[0]*3600 + parts[1]*60 + parts[2] + elapsed;
    total = ((total % 86400) + 86400) % 86400;
    var hh = String(Math.floor(total/3600)).padStart(2,'0');
    var mm = String(Math.floor((total%3600)/60)).padStart(2,'0');
    var ss = String(total%60).padStart(2,'0');
    clockEl.textContent = hh+':'+mm+':'+ss;
  }
  setInterval(tickClock, 1000);

  // ================= minimal MQTT 3.1.1 client over WebSocket =================
  function encodeUTF8String(str){
    var bytes = new TextEncoder().encode(str);
    var out = new Uint8Array(2 + bytes.length);
    out[0] = (bytes.length >> 8) & 0xFF;
    out[1] = bytes.length & 0xFF;
    out.set(bytes, 2);
    return out;
  }
  function encodeRemainingLength(len){
    var bytes = [];
    do {
      var b = len % 128;
      len = Math.floor(len / 128);
      if (len > 0) b |= 0x80;
      bytes.push(b);
    } while (len > 0);
    return bytes;
  }
  function readRemainingLength(buf, offset){
    var multiplier = 1, value = 0, i = offset, used = 0, byte;
    while (true){
      if (i >= buf.length) return null;
      byte = buf[i]; i++; used++;
      value += (byte & 0x7F) * multiplier;
      if ((byte & 0x80) === 0) break;
      multiplier *= 128;
      if (multiplier > 128*128*128) throw new Error('malformed remaining length');
    }
    return { value: value, bytesUsed: used };
  }
  function concatAll(arrays){
    var total = 0;
    for (var i=0;i<arrays.length;i++) total += arrays[i].length;
    var out = new Uint8Array(total);
    var off = 0;
    for (var j=0;j<arrays.length;j++){ out.set(arrays[j], off); off += arrays[j].length; }
    return out;
  }
  function u8(arr){ return new Uint8Array(arr); }

  function buildConnect(clientId, keepAlive){
    var protoName = encodeUTF8String('MQTT');
    var protoLevel = u8([4]);
    var connectFlags = u8([0x02]); // clean session, no will/user/pass
    var keepAliveBytes = u8([(keepAlive>>8)&0xFF, keepAlive & 0xFF]);
    var clientIdBytes = encodeUTF8String(clientId);
    var vhp = concatAll([protoName, protoLevel, connectFlags, keepAliveBytes, clientIdBytes]);
    var rl = encodeRemainingLength(vhp.length);
    return concatAll([u8([0x10].concat(rl)), vhp]);
  }
  function buildSubscribe(packetId, topic, qos){
    var pid = u8([(packetId>>8)&0xFF, packetId & 0xFF]);
    var topicBytes = encodeUTF8String(topic);
    var vhp = concatAll([pid, topicBytes, u8([qos])]);
    var rl = encodeRemainingLength(vhp.length);
    return concatAll([u8([0x82].concat(rl)), vhp]);
  }
  function buildPublish(topic, payload, retain){
    var topicBytes = encodeUTF8String(topic);
    var payloadBytes = new TextEncoder().encode(payload);
    var vhp = concatAll([topicBytes, payloadBytes]);
    var rl = encodeRemainingLength(vhp.length);
    var flags = 0x30 | (retain ? 0x01 : 0x00);
    return concatAll([u8([flags].concat(rl)), vhp]);
  }
  var PINGREQ = u8([0xC0, 0x00]);
  var DISCONNECT = u8([0xE0, 0x00]);

  function tryParsePacket(buf){
    if (buf.length < 2) return null;
    var rl = readRemainingLength(buf, 1);
    if (!rl) return null;
    var total = 1 + rl.bytesUsed + rl.value;
    if (buf.length < total) return null;
    return { packet: buf.slice(0, total), total: total, rlBytesUsed: rl.bytesUsed };
  }
  function parsePublish(packet, rlBytesUsed){
    var idx = 1 + rlBytesUsed;
    var topicLen = (packet[idx]<<8) | packet[idx+1]; idx += 2;
    var topic = new TextDecoder().decode(packet.slice(idx, idx+topicLen)); idx += topicLen;
    var qos = (packet[0] >> 1) & 0x3;
    if (qos > 0) idx += 2; // packet id, not expected at our QoS0 subscription
    var payload = new TextDecoder().decode(packet.slice(idx));
    return { topic: topic, payload: payload };
  }

  // ---------- link state ----------
  var ws = null, rxBuffer = new Uint8Array(0), pingTimer = null, reconnectTimer = null;
  var manualDisconnect = false, connected = false, nextPid = 1;
  var KEEP_ALIVE = 60;

  var statusChip = document.getElementById('statusChip');
  var statusText = document.getElementById('statusText');
  var linkBtn = document.getElementById('linkBtn');
  var brokerInput = document.getElementById('brokerUrl');
  var clientIdInput = document.getElementById('clientId');
  var autoReconnectChk = document.getElementById('autoReconnect');

  function setStatus(state, text){
    statusChip.setAttribute('data-state', state);
    statusText.textContent = text;
  }

  function randomClientId(){
    return 'webdash-' + Math.random().toString(16).slice(2, 8);
  }
  try {
    brokerInput.value = localStorage.getItem('iotlogger_broker') || brokerInput.value;
    clientIdInput.value = localStorage.getItem('iotlogger_client') || randomClientId();
  } catch(e){ clientIdInput.value = randomClientId(); }

  function connectMqtt(){
    manualDisconnect = false;
    var url = brokerInput.value.trim();
    var clientId = clientIdInput.value.trim() || randomClientId();
    try { localStorage.setItem('iotlogger_broker', url); localStorage.setItem('iotlogger_client', clientId); } catch(e){}

    setStatus('connecting', 'LINKING...');
    linkBtn.disabled = true;
    rxBuffer = new Uint8Array(0);

    try {
      ws = new WebSocket(url, 'mqtt');
    } catch (e) {
      // Two very different causes land here, so don't lump them into one
      // generic label: a malformed URL throws SyntaxError; opening ws://
      // from a page loaded over https:// throws SecurityError (mixed
      // content) even when the URL itself is perfectly correct.
      var isMixedContent = (window.location.protocol === 'https:' && /^ws:\/\//i.test(url));
      if (isMixedContent) {
        setStatus('offline', 'BLOCKED (https page, use ws:// only from file:// or http://)');
      } else if (e && e.name === 'SyntaxError') {
        setStatus('offline', 'BAD URL FORMAT');
      } else {
        setStatus('offline', 'CONNECT ERROR');
      }
      linkBtn.disabled = false;
      return;
    }
    ws.binaryType = 'arraybuffer';

    ws.onopen = function(){
      ws.send(buildConnect(clientId, KEEP_ALIVE));
    };
    ws.onmessage = function(ev){
      var incoming = new Uint8Array(ev.data);
      var merged = new Uint8Array(rxBuffer.length + incoming.length);
      merged.set(rxBuffer, 0); merged.set(incoming, rxBuffer.length);
      rxBuffer = merged;
      while (true){
        var res;
        try { res = tryParsePacket(rxBuffer); } catch(e){ rxBuffer = new Uint8Array(0); break; }
        if (!res) break;
        handlePacket(res.packet, res.rlBytesUsed);
        rxBuffer = rxBuffer.slice(res.total);
      }
    };
    ws.onclose = function(){
      connected = false;
      linkBtn.disabled = false;
      linkBtn.textContent = 'LINK';
      setStatus('offline', 'OFFLINE');
      if (pingTimer) { clearInterval(pingTimer); pingTimer = null; }
      if (!manualDisconnect && autoReconnectChk.checked){
        reconnectTimer = setTimeout(connectMqtt, 3000);
      }
    };
    ws.onerror = function(){ /* onclose follows */ };
  }

  function handlePacket(packet, rlBytesUsed){
    var type = packet[0] >> 4;
    if (type === 2) { // CONNACK
      var returnCode = packet[3];
      if (returnCode === 0){
        connected = true;
        linkBtn.disabled = false;
        linkBtn.textContent = 'UNLINK';
        setStatus('linked', 'LINKED');
        subscribe('logger/telemetry', 0);
        if (pingTimer) clearInterval(pingTimer);
        pingTimer = setInterval(function(){ if (ws && ws.readyState === 1) ws.send(PINGREQ); }, 20000);
      } else {
        setStatus('offline', 'REFUSED ('+returnCode+')');
        linkBtn.disabled = false;
      }
    } else if (type === 3) { // PUBLISH
      var msg = parsePublish(packet, rlBytesUsed);
      onMessage(msg.topic, msg.payload);
    }
    // SUBACK(9) / PINGRESP(13): no action needed
  }

  function subscribe(topic, qos){
    var pid = nextPid; nextPid = (nextPid % 65535) + 1;
    ws.send(buildSubscribe(pid, topic, qos));
  }
  function publish(topic, payload){
    if (!connected || !ws || ws.readyState !== 1) return false;
    ws.send(buildPublish(topic, payload, false));
    return true;
  }

  linkBtn.addEventListener('click', function(){
    if (connected || (ws && ws.readyState === WebSocket.CONNECTING)){
      manualDisconnect = true;
      if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
      try { if (ws.readyState === 1) ws.send(DISCONNECT); } catch(e){}
      try { ws.close(); } catch(e){}
    } else {
      connectMqtt();
    }
  });

  // ---------- UI wiring for telemetry ----------
  var msgCount = 0;
  var feedLog = document.getElementById('feedLog');
  var feedCount = document.getElementById('feedCount');

  function logFeed(text){
    if (feedLog.querySelector('.empty')) feedLog.innerHTML = '';
    var line = document.createElement('div');
    var t = new Date();
    var ts = String(t.getHours()).padStart(2,'0')+':'+String(t.getMinutes()).padStart(2,'0')+':'+String(t.getSeconds()).padStart(2,'0');
    var tSpan = document.createElement('span'); tSpan.className='t'; tSpan.textContent = '['+ts+'] ';
    line.appendChild(tSpan);
    line.appendChild(document.createTextNode(text));
    feedLog.appendChild(line);
    while (feedLog.children.length > 40) feedLog.removeChild(feedLog.firstChild);
    feedLog.scrollTop = feedLog.scrollHeight;
    msgCount++; feedCount.textContent = msgCount + ' MSG';
  }

  function setSensorTile(prefix, ok, temp, hum, pres){
    var pill = document.getElementById('pill'+prefix);
    var tile = document.getElementById('tile'+prefix);
    var tempEl = document.getElementById('temp'+prefix);
    var humEl = document.getElementById('hum'+prefix);
    var presEl = document.getElementById('pres'+prefix);
    if (ok){
      tile.classList.remove('fault');
      pill.setAttribute('data-ok','true'); pill.textContent = 'OK';
      tempEl.textContent = temp.toFixed(1);
      humEl.textContent = hum.toFixed(0) + ' %RH';
      presEl.textContent = pres.toFixed(0) + ' hPa';
    } else {
      tile.classList.add('fault');
      pill.setAttribute('data-ok','false'); pill.textContent = 'FAULT';
      tempEl.textContent = '--';
      humEl.textContent = '-- %RH';
      presEl.textContent = '-- hPa';
    }
  }

  var staleTimer = null;
  function armStaleWatch(){
    if (staleTimer) clearTimeout(staleTimer);
    staleTimer = setTimeout(function(){
      ['I2C','SPI'].forEach(function(p){
        var pill = document.getElementById('pill'+p);
        if (pill.getAttribute('data-ok') !== 'false') pill.setAttribute('data-ok','stale');
        pill.textContent = 'STALE';
      });
    }, 7000);
  }

  function onMessage(topic, payload){
    if (topic !== 'logger/telemetry') return;
    var data;
    try { data = JSON.parse(payload); } catch(e){ logFeed('RX malformed payload: '+payload); return; }

    logFeed(payload);
    armStaleWatch();

    if (data.utc_time){ lastUtc = data.utc_time; lastUtcAt = Date.now(); tickClock(); }

    setSensorTile('I2C', !!data.i2c_ok, Number(data.temp_i2c_c), Number(data.hum_i2c_rh), Number(data.press_i2c_hpa));
    setSensorTile('SPI', !!data.spi_ok, Number(data.temp_spi_c), Number(data.hum_spi_rh), Number(data.press_spi_hpa));

    if (typeof data.threshold_c === 'number'){
      var pct = Math.max(0, Math.min(100, (data.threshold_c - 15) / (35 - 15) * 100));
      document.getElementById('gaugeFill').style.width = pct + '%';
      document.getElementById('pillThresh').textContent = data.threshold_c.toFixed(0) + '°C';
      document.getElementById('pillThresh').setAttribute('data-ok', 'true');
    }

    if (typeof data.adc_raw === 'number') {
      document.getElementById('adcRaw').textContent = data.adc_raw + ' / 4095';
    }

    var ledOn = !!data.led_on;
    document.getElementById('ledLamp').setAttribute('data-on', ledOn ? 'true':'false');
    document.getElementById('ledState').textContent = ledOn ? 'ON' : 'OFF';
    document.getElementById('ledSource').textContent = data.led_source ? String(data.led_source).toUpperCase() : '--';
  }

  document.getElementById('ledOnBtn').addEventListener('click', function(){
    if (publish('logger/control/led', 'ON')) logFeed('TX logger/control/led = ON');
  });
  document.getElementById('ledOffBtn').addEventListener('click', function(){
    if (publish('logger/control/led', 'OFF')) logFeed('TX logger/control/led = OFF');
  });

})();
```

### Повний робочий процес — від живлення плати до цифр на екрані комп'ютера

Це не три незалежні дії, а один послідовний ланцюжок; кожен наступний
крок має сенс лише якщо попередній справді завершився.

```
Телефон ──Wi-Fi──▶ ESP32 (тимчасово сам стає точкою доступу)   [лише при першому налаштуванні, Крок 8]
                          │ застосунок ESP SoftAP Provisioning передає SSID/пароль
                          ▼
ESP32 ──Wi-Fi (STA)──▶ Ваш роутер ──▶ Комп'ютер з Mosquitto (брокер, Крок 8)
                                                │
Браузер на комп'ютері (dashboard.html) ─────────┘  (той самий брокер, той самий LAN, Челендж 7)
```

1. **Прошийте плату** (`idf.py -p COMx flash monitor`) і пройдіть Wi-Fi
   provisioning рівно один раз (Завдання 8.0.1) — доки в консолі не
   з'явиться `WIFI_PROV: Wi-Fi ready` і слідом `MQTT_LINK: MQTT connected
   to ...`.
2. **На комп'ютері з Mosquitto** переконайтесь, що в `mosquitto.conf` є
   обидва listener'и (`1883` для плати, `9001`/`websockets` для
   браузера, вище в цьому Челенджі) і брокер перезапущений саме з цим
   конфігом. Дізнайтесь IP цього комп'ютера (`ipconfig`/`ip addr`) — він
   має бути в **тій самій** Wi-Fi мережі, до якої щойно підключилась
   плата.
3. **Відкрийте `web/templates/html/dashboard.html`** подвійним кліком —
   локально, без інтернету.
4. У полі **Broker (WebSocket)** впишіть `ws://<IP з кроку 2>:9001`,
   натисніть **LINK**.
5. Статус-чіп мусить стати `LINKED` (зелена крапка) — і протягом
   ~2 секунд з'являться перші показання: обидва BME280, поріг з ручки,
   стан LED. Кнопки **ARM(ON)/SAFE(OFF)** шлють ту саму команду, що й
   `mosquitto_pub -t "logger/control/led"` у Завданні 8.3.

### Діагностика самої веб-консолі (доповнення до 12.1, Симптом Г)

- **Статус-чіп назавжди лишається `OFFLINE`/`LINKING...`.** Перевірте
  трьома незалежними кроками: (а) чи взагалі є `listener 9001` /
  `protocol websockets` у конфізі, з яким **фактично** запущено
  Mosquitto (не в старому файлі поруч); (б) чи не заблокував браузер
  з'єднання як **mixed content** — сторінка відкрита по `https://`
  (наприклад, онлайн-прев'ю), а брокер — по `ws://` без TLS; рішення —
  відкривати `dashboard.html` локально (`file://`) чи зі свого LAN,
  а не з хмарного хостингу (в консолі розробника браузера, F12, така
  помилка видна дослівно як "Mixed Content"); (в) правильний ЛИШЕ IP,
  не `localhost` — брокер і плата в одній Wi-Fi мережі, а комп'ютер із
  браузером може бути третім окремим пристроєм у тій самій мережі.
- **`LINKED`, але плитки датчиків показують `NO DATA`/`FAULT` — цифр
  немає.** Це означає, що WebSocket-з'єднання з брокером **є**, але
  повідомлення в `logger/telemetry` або взагалі не приходять (плата ще
  не дійшла до `while(1)` в `app_main()` — перевірте лог плати окремо,
  Симптом Б у 12.1), або `i2c_ok`/`spi_ok` у JSON — `false` (сам датчик
  зараз не відповідає — Симптом А в 12.1, лише на стороні плати, до
  веб-консолі це відношення не має).
- **Плитка раптом стає `STALE` (жовта), хоча раніше показувала `OK`.**
  Це власна watchdog-логіка сторінки (`armStaleWatch()` у
  `dashboard.js`) — понад 7 секунд без нового `logger/telemetry`;
  означає, що плата або втратила Wi-Fi/MQTT (перевірте
  `mqtt_link_is_connected()` в її власному консольному логу), або її
  просто вимкнули/перезавантажили.

---

## ✅ ПІДСУМОК: ЩО МАЄТЕ ОТРИМАТИ В КІНЦІ

- [ ] Модульний ESP-IDF-проєкт (`iot_logger`) з одинадцятьма незалежними
      компонентами (`i2c_bus`, `env_i2c`, `env_spi`, `bme280_compensate`,
      `rtc_ds1307`, `u8g2_hal`, `oled_display`, `ctrl_adc`, `led_ctrl`,
      `wifi_prov`, `mqtt_link`), об'єднаними лише через `main/app_main.c`.
- [ ] (Челендж 7, опційно) Автономна веб-консоль телеметрії на чистому
      MQTT-over-WebSocket — той самий `logger/telemetry`/`logger/control/led`,
      що й `mosquitto_sub`/`mosquitto_pub`, лише з браузера.
- [ ] Wi-Fi без жодного пароля в коді: `wifi_prov_mgr`+SoftAP, телефон із
      застосунком ESP SoftAP Provisioning передає реальні дані мережі
      через захищену `protocomm`-сесію рівно один раз у житті пристрою.
- [ ] Один і той самий фізичний параметр, прочитаний одночасно двома
      незалежними шинами (I²C і SPI, Модулі 4.2-4.5) з тим самим датчиком
      BME280.
- [ ] Три пристрої на одній спільній I²C-шині (Модуль 4.2-4.3), керовані
      через єдиний `i2c_bus`-компонент — без повторної ініціалізації шини
      з трьох різних файлів.
- [ ] Дисплей із власною вступною анімацією (вовк, точно 5 секунд, Модуль
      4.3) і живим дашбордом часу й показань обох датчиків.
- [ ] Апаратний годинник реального часу (DS1307), що тримає час навіть
      між перезавантаженнями, "засіяний" один раз через SNTP.
- [ ] Розуміння різниці "гучної" помилки протоколу (I²C NACK, Модуль 4.3)
      і "тихої" помилки даних (власний `t_fine` на датчик, Модуль 4.5).
- [ ] Контрольну ручку на `adc_continuous`+DMA (Модуль 4.6) без жодного
      опитування CPU головним циклом.
- [ ] Робочий MQTT pub/sub з нуля на власному брокері (Mosquitto) через
      `esp-mqtt`: publish телеметрії, subscribe на команду керування, і
      SNTP-синхронізація для "засівання" Tiny RTC реальним часом.
- [ ] Досвід живого дебагу вбудованим `USB-Serial-JTAG` (Модуль 4.7) на
      реальній, багатозадачній системі — включно з `SMP debug`.
- [ ] Ви можете пояснити словами, чому "Логер Даних та Керування" — не два
      окремі завдання, а одна система: одна температура надходить двома
      незалежними шляхами для перевірки, час тримається незалежно від
      мережі, а один LED-індикатор слухає два незалежні джерела команди.

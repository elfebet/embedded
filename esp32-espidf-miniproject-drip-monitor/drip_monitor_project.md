# 🏥 💧 🚨 ПРОЄКТ 7 · HEALTH · MEDICAL · ~6 ГОД · ESP-IDF v5.5.4

# Тренажер монітора інфузії: лічильник крапель, затискач і аварійний зупин

> Покрокова практика: на ESP32-S3 (**чистий ESP-IDF, без Arduino**) будуємо
> навчальний тренажер приладу, що контролює темп крапельниці — той самий
> клас логіки, що в реальних інфузійних насосах рахує краплі й зупиняє потік
> при відхиленні від норми. Оптичний "краплемір" (фоторезистор у щілині)
> дає сирий сигнал АЦП (Модуль 3.1), який з гістерезисом перетворюється на
> події "крапля", а частота подій — на крапель/хвилину з бюджетом похибки
> (Модуль 3.2). Термістор дає температуру "пацієнта" тим самим шляхом.
> Оператор задає цільовий темп енкодером (Модуль 3.6), серво-затискач
> (Модуль 3.5) відкриває/закриває трубку, ШІМ-зумер (Модуль 3.3) і
> SDM-псевдо-ЦАП (Модуль 3.4) сигналізують про відхилення. **Це виключно
> навчальний тренажер на макетній платі** — жодне реальне медичне обладнання
> чи пацієнти тут не задіяні.

**🧩 ESP-IDF з нуля · 🏗️ Модульні компоненти · 🎚️ ADC-калібрування (2 канали) · 🔄 PCNT-енкодер · 🎯 Серво-затискач · 📶 SDM-псевдо-ЦАП · 🏆 Челенджі**

*Інтегрує Модулі 3.1 (аналогові сигнали/АЦП, гістерезис), 3.2 (калібрування, бюджет похибки), 3.3 (ШІМ), 3.4 (ЦАП/SDM), 3.5 (серво через ШІМ-регістри LEDC), 3.6 (енкодер/PCNT).*

---

## 🎯 МЕТА ПРОЄКТУ

**У кінці ви отримаєте:**

1. Модульний ESP-IDF-проєкт (`components/…`): `drop_sensor` (АЦП + гістерезис +
   темп крапель), `temp_sensor` (АЦП + калібрування NTC), `rate_dial`
   (PCNT-енкодер), `clamp_servo` (LEDC-серво затискача), `alarm_driver`
   (ШІМ-зумер + SDM-LED) — кожен відповідає за одну periferію.
2. Робочий лічильник крапель на аналоговому порозі з гістерезисом — без
   гістерезису порогове значення "деренчить" і рахує зайві події на кожній
   краплі, що фізично неприпустимо для дозування.
3. Калібровану температуру за B-параметричною формулою термістора з явним
   бюджетом похибки — тут ціна помилки вимірювання не абстрактна, а пряма
   аналогія до клінічної точності.
4. Задання цільового темпу крапель обертанням енкодера, з негайним
   візуальним/звуковим відгуком на відхилення.
5. Серво-затискач, кероване напряму через `ledc_set_duty()` — і, найголовніше,
   **безпечну поведінку за замовчуванням**: при старті й при відмові
   датчика затискач іде в положення "закрито", а не "відкрито".
6. Розуміння, чому "тиха" відмова датчика (обрив, NaN) у медичному
   застосунку — це не дрібниця, а вимога до архітектури: система повинна
   зупиняти потік, а не продовжувати працювати "наосліп".

---

## 01 · Крок 0 · Обладнання, схема, безпека ⏱ 30 хв

**Що на столі:** ESP32-S3-DevKitC-1 N16R8 (USB-C, кабель **даних**), макетна
плата, перемички, фоторезистор (LDR), резистор **10 кОм** (дільник LDR),
термістор NTC **10 кОм** (B=3950) з резистором **10 кОм** (дільник NTC),
роторний енкодер **KY-040**, мікросервопривод **SG90** з окремим 5В
живленням, BC547B, резистори **220 Ом**, **330 Ом**, **10 кОм**, конденсатор
**100 нФ**, звичайний LED, пасивний зумер, логічний аналізатор.

### Список компонентів і посилання на покупку

| Компонент | Магазин | Посилання |
|---|---|---|
| ESP32-S3-DevKitC-1 N16R8 | Arduino.ua | https://arduino.ua/ru/prod7600-plata-razrabotchika-esp32-s3-n16r8-type-c |
| Макетна плата (830 точок) | ArduShop | https://ardushop.in.ua/arduino/prototyping-board-without-soldering-mb-102-830-points |
| Набір проводів-перемичок (M-M, M-F) | Mini-Tech | https://www.mini-tech.com.ua/ua/provoda-dlya-maketirovaniya-papa-mama |
| Фоторезистор GL5528 5 мм | ArduShop | https://ardushop.in.ua/radio-components/photoresistor-gl5528 |
| Терморезистор NTC 10к, B=3950 (MF52) | RadioMAG | https://www.rcscomponents.kiev.ua/product/ntc-termistor-10-kohm-1-vyv-mf52-103f3950_122836.html |
| Енкодер KY-040 | Arduino.ua | https://arduino.ua/prod1223-modyl-enkoder |
| Мікросервопривод SG90 | Mini-Tech | https://www.mini-tech.com.ua/servomotor-sg90 |
| Транзистор BC547B (NPN) | RadioMAG | https://www.rcscomponents.kiev.ua/product/bc547b-tranzistor-bipolyarnij-npn_15291.html |
| Пасивний п'єзо-зумер 5В | Mini-Tech | https://www.mini-tech.com.ua/ua/passivnyj-zummer |
| Світлодіод 5 мм (червоний) | Arduino.ua | https://arduino.ua/prod372-Svetodiod_krasnii |
| Набір резисторів (220/330/10к і ін.) | Geekmatic | https://geekmatic.in.ua/ua/resistor_kit_30_values |
| Керамічний конденсатор 100 нФ | BlackChip | https://blackchip.com.ua/kondensator-keramichniy-100nf-50v-rm5/ |
| Логічний аналізатор 24 МГц/8 каналів | Rozetka | https://rozetka.com.ua/ua/282178298/p282178298/ |
| Тримач 4×AA (живлення серво) | BestBattery | https://bestbattery.com.ua/ua/aa_battery_ua/holder_contacts_aa_ua/holder_4xaa_s_ua |

> **🟠 МІНІ-ЗАВДАННЯ**
> Ціни й наявність у магазинах змінюються — перш ніж замовляти, звірте
> актуальну ціну й характеристики прямо на сторінці товару.

> **🔴 ЦЕ ТРЕНАЖЕР, НЕ МЕДИЧНИЙ ПРИЛАД**
> Серво тут рухає лише вільний важіль на макетній платі, що умовно
> "затискає" уявну трубку — жодної реальної інфузійної системи, голок чи
> рідин у цій практиці немає. Метою є засвоєння принципів вимірювання й
> замкненого контуру керування, а не побудова медичного пристрою.

```
ПІНАУТ ПРОЄКТУ (ESP32-S3, безпечні GPIO — уникаємо 0/3/19/20/26-32/39-46)

  GPIO4  ── вхід ADC1_CH3     ← дільник LDR (краплемір): LDR+10кОм
  GPIO5  ── вхід ADC1_CH4     ← дільник NTC (температура): NTC+10кОм
  GPIO6  ── вхід (pull-up)    ← KY-040 CLK (канал A) — набір цільового темпу
  GPIO7  ── вхід (pull-up)    ← KY-040 DT  (канал B)
  GPIO8  ── вхід (pull-up)    ← KY-040 SW  (кнопка "озвучити тривогу/тиша")
  GPIO15 ── вихід LEDC        → сигнал SG90 (затискач трубки)
  GPIO16 ── вихід LEDC        → база BC547B (220 Ом) → зумер тривоги
  GPIO17 ── вихід SDM         → RC-фільтр (10 кОм / 100 нФ) → LED (330 Ом) — "темп потоку"
```

### Схема складання на макетній платі ⏱ додатково 15 хв

```
                 ЛІВА СИЛОВА РЕЙКА (+ червона = 3.3V, − синя = GND)
   +  ─────────────────────────────────────────────────────────────────
   −  ─────────────────────────────────────────────────────────────────
        a  b  c  d  e     f  g  h  i  j
   1     .  .  .  .  .     .  .  .  .  .
   2     .  .  .  .  .     .  .  .  .  .   ← LDR: одна ніжка → рейка (+), інша → c2
   3     .  .  .  .  .     .  .  .  .  .     резистор 10кОм: c2→c3→рейка(−)
   4     .  .  .  .  .     .  .  .  .  .     перемичка f2→GPIO4 (вузол LDR/резистор)
   5     .  .  .  .  .     .  .  .  .  .
   6     .  .  .  .  .     .  .  .  .  .   ← NTC: одна ніжка → рейка (+), інша → c6
   7     .  .  .  .  .     .  .  .  .  .     резистор 10кОм: c6→c7→рейка(−)
   8     .  .  .  .  .     .  .  .  .  .     перемичка f6→GPIO5 (вузол NTC/резистор)
   9     .  .  .  .  .     .  .  .  .  .
  10     .  .  .  .  .     .  .  .  .  .   ← KY-040: + →рейка(+), GND→рейка(−),
  11     .  .  .  .  .     .  .  .  .  .     CLK→GPIO6, DT→GPIO7, SW→GPIO8
  12     .  .  .  .  .     .  .  .  .  .
  13     .  .  .  .  .     .  .  .  .  .   ← BC547B (зумер): аналогічно Кроку 0 Проєкту 6,
  14     .  .  .  .  .     .  .  .  .  .     база через 220 Ом → GPIO16
  15     .  .  .  .  .     .  .  .  .  .
  16     .  .  .  .  .     .  .  .  .  .   ← RC-фільтр SDM → LED: GPIO17→10кОм→вузол Y,
  17     .  .  .  .  .     .  .  .  .  .     100нФ Y→рейка(−), LED (330 Ом) з вузла Y
  18     .  .  .  .  .     .  .  .  .  .
        a  b  c  d  e     f  g  h  i  j
   +  ─────────────────────────────────────────────────────────────────
   −  ─────────────────────────────────────────────────────────────────

  SG90: сигнал → GPIO15; живлення — ОКРЕМЕ 5В джерело, спільний GND з платою.
```

**Порядок складання.**
1. `GND`→синя рейка, `3V3`→червона рейка ESP32-S3. Прозвінка на КЗ.
2. Дільник LDR і дільник NTC — за схемою вище (обидва — класична схема
   "верхнє плече датчик, нижнє плече фіксований резистор", вихід — з
   середньої точки на АЦП).
3. KY-040, BC547B+зумер, RC-фільтр+LED — за аналогією з Проєктом 6 (той
   самий підхід, інші GPIO).
4. SG90 — сигнал на GPIO15, живлення від окремого джерела, спільний GND.
5. Повторна прозвінка на КЗ — і лише тоді підключення USB та живлення серво.

> **🔴 ТЕХНІКА БЕЗПЕКИ**
> Перемикання — лише при вийнятому USB і відключеному живленні серво.
> LDR чутливий до навколишнього освітлення — виконуйте Крок 3 при стабільному
> світлі в кімнаті (не біля вікна з мінливими хмарами), інакше поріг
> гістерезису доведеться підбирати заново.

**ПЛАН ПРАКТИКИ**

| Час | Крок | Що робимо |
|---|---|---|
| 0:00–0:30 | Крок 0 | Схема, безпека |
| 0:30–0:55 | Крок 1 | Ініціалізація ESP-IDF проєкту |
| 0:55–1:20 | Крок 2 | Модульна архітектура |
| 1:20–2:00 | Крок 3 | АЦП краплеміра: сирий сигнал і гістерезис (Модуль 3.1) |
| 2:00–2:45 | Крок 4 | Калібрування: крапель/хв і температура NTC (Модуль 3.2) |
| 2:45–3:15 | Крок 5 | ШІМ-зумер тривоги (Модуль 3.3) |
| 3:15–3:50 | Крок 6 | SDM-псевдо-ЦАП: індикатор темпу потоку (Модуль 3.4) |
| 3:50–4:30 | Крок 7 | Серво-затискач через LEDC, безпечний стан (Модуль 3.5) |
| 4:30–5:10 | Крок 8 | PCNT-енкодер: набір цільового темпу (Модуль 3.6) |
| 5:10–5:45 | Крок 9 | Інтеграція: контур керування темпом |
| 5:45–6:00 | Крок 10 | Просте тестування |
| 6:00–6:35 | Челенджі | 5 завдань на розширення |

---

## 02 · Крок 1 · Ініціалізація проєкту ESP-IDF ⏱ 25 хв

> **🟢 МЕТА**
> Створити самостійний ESP-IDF-проєкт `drip_monitor`.

```bash
idf.py create-project drip_monitor
cd drip_monitor
idf.py set-target esp32s3
mkdir -p components/drop_sensor/include components/temp_sensor/include ^
         components/rate_dial/include components/clamp_servo/include ^
         components/alarm_driver/include
```

> **🔵 ЩО СПОСТЕРІГАЄМО**
> `idf.py build` поки не проходить — компоненти отримають вміст у наступних
> кроках.

---

## 03 · Крок 2 · Модульна архітектура ⏱ 25 хв

> **🟢 МЕТА**
> П'ять компонентів, кожен — одна periferія, без жодного слова "пацієнт",
> "доза" чи "тривога" всередині них самих.

```
drip_monitor/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── app_main.c              ← Крок 9: контур керування темпом
└── components/
    ├── drop_sensor/             ← Кроки 3-4: АЦП + гістерезис + крапель/хв
    ├── temp_sensor/              ← Крок 4: АЦП + калібрування NTC
    ├── rate_dial/                 ← Крок 8: PCNT-енкодер цільового темпу
    ├── clamp_servo/                ← Крок 7: LEDC-серво затискача
    └── alarm_driver/                ← Кроки 5-6: ШІМ-зумер + SDM-LED
```

**`main/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES drop_sensor temp_sensor rate_dial clamp_servo alarm_driver
)
```

> **🟣 РОЗБІР: чому саме так**
> `clamp_servo` нічого не знає про темп крапель, `drop_sensor` нічого не
> знає про серво — рішення "закрити затискач, бо темп занадто високий"
> приймається виключно в `main/app_main.c` (Крок 9). Такий поділ дозволяє
> протестувати кожен компонент окремо, не чекаючи, поки готова вся система.

> **🟠 МІНІ-ЗАВДАННЯ**
> Який з п'яти компонентів ви б узяли без змін для зовсім іншого проєкту
> (наприклад, автоматичного годівника для тварин з дозуванням по краплях
> корму)?

---

## 04 · Крок 3 · АЦП краплеміра: сирий сигнал і гістерезис ⏱ 40 хв

> **🟢 МЕТА**
> Побачити сирий АЦП-сигнал з фотодільника й перетворити його на чіткі
> події "крапля" через поріг із гістерезисом (Модуль 3.1) — без гістерезису
> шум на межі порогу рахує кілька "крапель" замість однієї.

**`components/drop_sensor/include/drop_sensor.h`:**
```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DROP_ADC_CHANNEL   ADC_CHANNEL_3   // GPIO4

void     drop_sensor_init(void);
uint16_t drop_sensor_read_raw(void);
bool     drop_sensor_poll_event(void);       // true рівно раз на кожну виявлену краплю
float    drop_sensor_get_rate_dpm(void);     // Крок 4: крапель/хвилину
bool     drop_sensor_is_stalled(uint32_t timeout_ms);  // Крок 9: немає крапель довше timeout

#ifdef __cplusplus
}
#endif
```

**`components/drop_sensor/drop_sensor.c` (частина 1 — сирий АЦП і гістерезис):**
```c
#include "drop_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "drop_sensor";
static adc_oneshot_unit_handle_t s_adc_handle;

// Гістерезис: крапля, що перетнула світловий промінь, дає ПРОВАЛ напруги
// (менше світла на LDR -> більший опір -> менша напруга на дільнику, якщо
// LDR у верхньому плечі). Два різні пороги замість одного не дають сигналу
// "деренчати" навколо межі.
#define THRESH_LOW    1500   // нижче цього — "крапля в промені" (подія готується)
#define THRESH_HIGH   2000   // вище цього — "промінь чистий" (подія завершена, готові до наступної)

static bool s_armed = true;   // true = чекаємо на провал (початок нової краплі)

void drop_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(s_adc_handle, DROP_ADC_CHANNEL, &chan_cfg);
}

uint16_t drop_sensor_read_raw(void) {
    int raw = 0;
    adc_oneshot_read(s_adc_handle, DROP_ADC_CHANNEL, &raw);
    return (uint16_t)raw;
}

bool drop_sensor_poll_event(void) {
    uint16_t raw = drop_sensor_read_raw();

    if (s_armed && raw < THRESH_LOW) {
        s_armed = false;         // подія зафіксована, чекаємо повернення вище THRESH_HIGH
        return true;
    }
    if (!s_armed && raw > THRESH_HIGH) {
        s_armed = true;          // промінь знову чистий — готові рахувати наступну краплю
    }
    return false;
}
```

**`components/drop_sensor/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "drop_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_timer
)
```

**Тимчасовий тест — `main/app_main.c` (буде замінений у Кроці 9):**
```c
#include "drop_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "raw_test";

void app_main(void) {
    drop_sensor_init();
    while (1) {
        if (drop_sensor_poll_event()) {
            ESP_LOGI(TAG, "КРАПЛЯ! raw=%u", drop_sensor_read_raw());
        }
        vTaskDelay(pdMS_TO_TICKS(5));   // швидкий опит — крапля коротка
    }
}
```

> **🟣 РОЗБІР: чому саме так**
> - **Два пороги (`THRESH_LOW`/`THRESH_HIGH`), а не один** — якщо сирий
>   сигнал коливається біля єдиного порогу (шум АЦП, тремтіння руки при
>   імітації краплі пальцем), одна фізична подія рахується як кілька — той
>   самий клас проблеми, що й дребезг механічної кнопки (Модуль 2.6), тільки
>   в аналоговому, а не цифровому світі.
> - **`s_armed` — прапорець стану, а не просто "raw < поріг"** — подія
>   рахується лише на переході "чисто → закрито", і наступна подія
>   неможлива, поки сигнал не повернувся вище `THRESH_HIGH`. Це і є суть
>   гістерезису: різні пороги для входу й виходу зі стану.
> - **Опит кожні 5 мс, а не 50-100 мс** — реальна крапля перекриває промінь
>   на десятки мілісекунд; занадто рідкий опит просто пропустить подію
>   повністю, а не порахує її кілька разів.

> **🔵 ЩО СПОСТЕРІГАЄМО**
> Піднесіть палець близько до LDR і швидко приберіть (імітація краплі) —
> у лозі одна подія "КРАПЛЯ!" на один рух, незалежно від того, наскільки
> "тремтливо" ви це зробили.

> **🟠 МІНІ-ЗАВДАННЯ**
> Приберіть тимчасово `THRESH_HIGH` (порівнюйте з тим самим `THRESH_LOW` в
> обидва боки) і подивіться, скільки подій дає один повільний рух пальця
> біля межі порогу. Поверніть гістерезис на місце.

---

## 05 · Крок 4 · Калібрування: крапель/хв і температура NTC ⏱ 45 хв

> **🟢 МЕТА**
> Перевести події крапель у крапель/хвилину з бюджетом похибки, і сирий АЦП
> термістора — у градуси Цельсія за B-параметричною формулою (Модуль 3.2).

**`components/drop_sensor/drop_sensor.c` (частина 2 — темп, додати в кінець файлу):**
```c
#define RATE_WINDOW_N   5     // ковзне середнє по 5 останніх інтервалах — менш "нервова" оцінка

static int64_t s_last_event_us = 0;
static float   s_intervals_s[RATE_WINDOW_N] = {0};
static int     s_interval_idx = 0;
static int64_t s_last_seen_event_us = 0;   // для drop_sensor_is_stalled()

// Викликати ЩОРАЗУ, коли drop_sensor_poll_event() повернув true (Крок 9)
void drop_sensor_register_event(void) {
    int64_t now_us = esp_timer_get_time();
    s_last_seen_event_us = now_us;

    if (s_last_event_us != 0) {
        float dt_s = (now_us - s_last_event_us) / 1e6f;
        if (dt_s > 0.05f && dt_s < 10.0f) {   // відкидаємо нефізичні викиди (< 50 мс, > 10 с)
            s_intervals_s[s_interval_idx] = dt_s;
            s_interval_idx = (s_interval_idx + 1) % RATE_WINDOW_N;
        }
    }
    s_last_event_us = now_us;
}

float drop_sensor_get_rate_dpm(void) {
    float sum = 0;
    int n = 0;
    for (int i = 0; i < RATE_WINDOW_N; i++) {
        if (s_intervals_s[i] > 0) { sum += s_intervals_s[i]; n++; }
    }
    if (n == 0) return 0.0f;
    float avg_interval_s = sum / n;
    return 60.0f / avg_interval_s;   // крапель за хвилину
}

bool drop_sensor_is_stalled(uint32_t timeout_ms) {
    if (s_last_seen_event_us == 0) return false;   // ще жодної краплі — не "зупинка", а "старт"
    int64_t elapsed_ms = (esp_timer_get_time() - s_last_seen_event_us) / 1000;
    return elapsed_ms > (int64_t)timeout_ms;
}
```

**`components/temp_sensor/include/temp_sensor.h`:**
```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void  temp_sensor_init(void);
bool  temp_sensor_read_celsius(float *out_c, float *out_err_c);

#ifdef __cplusplus
}
#endif
```

**`components/temp_sensor/temp_sensor.c`:**
```c
#include "temp_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <math.h>

#define TEMP_ADC_CHANNEL   ADC_CHANNEL_4   // GPIO5

// Дільник: NTC у ВЕРХНЬОМУ плечі (живлення->NTC->вузол->10кОм->GND).
// B-параметрична формула термістора (спрощене рівняння Стейнхарта-Харта):
// 1/T = 1/T0 + (1/B)*ln(R/R0)
#define NTC_R0_OHM     10000.0f
#define NTC_B          3950.0f
#define NTC_T0_K       298.15f   // 25°C
#define DIVIDER_R_OHM  10000.0f

static adc_oneshot_unit_handle_t s_adc_handle;

void temp_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL, &chan_cfg);
}

bool temp_sensor_read_celsius(float *out_c, float *out_err_c) {
    int raw = 0;
    adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL, &raw);

    if (raw <= 5 || raw >= 4090) {   // обрив або коротке замикання дільника
        return false;
    }

    // raw/4095 ~ частка напруги живлення на нижньому резисторі (10кОм).
    // R_ntc = R_divider * (Vcc/V_node - 1), де V_node/Vcc = raw/4095
    float ratio = (float)raw / 4095.0f;
    float r_ntc = DIVIDER_R_OHM * (1.0f / ratio - 1.0f);

    float inv_t = (1.0f / NTC_T0_K) + (1.0f / NTC_B) * logf(r_ntc / NTC_R0_OHM);
    float temp_c = (1.0f / inv_t) - 273.15f;

    // Бюджет похибки: ±½ LSB на raw (0.4 мВ при ADC_ATTEN_DB_12) поширений
    // через похідну нелінійної формули термістора — тут спрощено оцінений
    // як стала ±0.3°C у робочому діапазоні 20-45°C (реальний розрахунок
    // похідної dT/dR — Челендж 2).
    *out_c = temp_c;
    *out_err_c = 0.3f;
    return true;
}
```

**`components/temp_sensor/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "temp_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`drop_sensor_register_event()` — окрема функція, яку викликає `main.c`,
>   а не сам `drop_sensor_poll_event()`** — підрахунок темпу є окремою
>   відповідальністю від виявлення факту краплі; такий поділ дозволяє
>   Кроку 9 вирішувати, коли саме подія "рахується" в темп (наприклад,
>   ігнорувати перші краплі після відкриття затискача).
> - **Ковзне середнє по 5 інтервалах, а не миттєвий `60/dt` на кожній
>   краплі** — миттєва оцінка занадто "нервова" для індикації оператору:
>   одна крапля на 0.1с раніше/пізніше дає стрибок в десятки крапель/хв на
>   миттєвій формулі.
> - **`temp_sensor_read_celsius()` повертає `bool` і явно перевіряє межові
>   значення `raw`** — обрив дроту (raw≈4095) чи коротке замикання (raw≈0)
>   не повинні мовчки дати "правдоподібну" аномальну температуру; викликач
>   (Крок 9) зобов'язаний перевірити повернене значення.
> - **`out_err_c` повертається поруч зі значенням, а не окремою константою
>   десь у документації** — виклик коду й контракт похибки завжди разом,
>   неможливо забути, яка похибка відповідає якому виміру.

> **🔵 ЩО СПОСТЕРІГАЄМО**
> Кілька повільних "крапель" пальцем дають стабільне число крапель/хв, що
> зростає з частішанням рухів. Термістор при кімнатній температурі показує
> ≈20-25°C; тепле дихання чи пальці на кілька секунд піднімають показник на
> кілька градусів і повільно повертають назад.

> **🟠 МІНІ-ЗАВДАННЯ**
> Порахуйте вручну `r_ntc` при `raw = 2048` (рівно половина шкали) і звірте
> з `temp_c`, який виводить лог. Чи логічно, що `raw = 2048` не обов'язково
> відповідає рівно 25°C (за яких `R = R0`)?

---

## 06 · Крок 5 · ШІМ-зумер тривоги ⏱ 25 хв

> **🟢 МЕТА**
> Ідентично до Кроку 5 Проєкту 6 (LEDC, 10-бітна роздільність, керована
> частота й гучність), тепер — сигнал відхилення темпу від цілі.

**`components/alarm_driver/include/alarm_driver.h`:**
```c
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void alarm_driver_init(void);
void alarm_buzzer_set(uint32_t freq_hz, uint8_t volume_percent);
void alarm_led_set_level(uint8_t percent);   // Крок 6

#ifdef __cplusplus
}
#endif
```

**`components/alarm_driver/alarm_driver.c` (частина 1 — ШІМ-зумер):**
```c
#include "alarm_driver.h"
#include "driver/ledc.h"

#define BUZZER_PIN     GPIO_NUM_16
#define BUZZER_TIMER   LEDC_TIMER_0
#define BUZZER_MODE    LEDC_LOW_SPEED_MODE
#define BUZZER_CHANNEL LEDC_CHANNEL_0
#define BUZZER_RES     LEDC_TIMER_10_BIT

void alarm_driver_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BUZZER_MODE, .duty_resolution = BUZZER_RES,
        .timer_num = BUZZER_TIMER, .freq_hz = 1000, .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .gpio_num = BUZZER_PIN, .speed_mode = BUZZER_MODE,
        .channel = BUZZER_CHANNEL, .timer_sel = BUZZER_TIMER,
        .duty = 0, .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);
}

void alarm_buzzer_set(uint32_t freq_hz, uint8_t volume_percent) {
    if (volume_percent > 100) volume_percent = 100;
    ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq_hz);
    uint32_t max_duty = (1u << BUZZER_RES) - 1u;
    uint32_t duty = (max_duty * volume_percent) / 100u / 2u;
    ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, duty);
    ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
}
```

**`components/alarm_driver/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "alarm_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc esp_driver_sdm
)
```

> **🟣 РОЗБІР: чому саме так**
> У медичному контексті гучність тривоги **зростає ступінчасто** (Крок 9),
> а не миттєво до максимуму — легке відхилення дає тихий попереджувальний
> тон, значне — гучну безперервну тривогу. Цей компонент лише надає важіль
> (частота/гучність), рішення "наскільки гучно" — бізнес-логіка Кроку 9.

> **🔵 ЩО СПОСТЕРІГАЄМО**
> Ідентично Проєкту 6 — тимчасовий виклик `alarm_buzzer_set(1500, 40)` дає
> чутний попереджувальний тон.

> **🟠 МІНІ-ЗАВДАННЯ**
> Чому в медичному тренажері "ступінчаста" гучність (кілька рівнів) краща
> за бінарну "тихо/гучно"? Наведіть аналогію з реальним обладнанням, яке ви
> бачили чи чули (побутове чи інше).

---

## 07 · Крок 6 · SDM-псевдо-ЦАП: індикатор темпу потоку ⏱ 35 хв

> **🟢 МЕТА**
> Вивести аналоговий рівень, пропорційний поточному темпу крапель, на GPIO
> через SDM (Модуль 3.4 — на ESP32-S3 апаратного ЦАП немає, SDM+RC-фільтр —
> офіційна заміна, ідентично Кроку 6 Проєкту 6).

**`components/alarm_driver/alarm_driver.c` (частина 2 — SDM, додати в кінець файлу):**
```c
#include "driver/sdm.h"

#define FLOW_LED_PIN   GPIO_NUM_17
#define SDM_SAMPLE_HZ  500000

static sdm_channel_handle_t s_sdm_chan = NULL;

static void flow_led_sdm_init(void) {
    sdm_config_t cfg = {
        .clk_src = SDM_CLK_SRC_DEFAULT,
        .gpio_num = FLOW_LED_PIN,
        .sample_rate_hz = SDM_SAMPLE_HZ,
    };
    sdm_new_channel(&cfg, &s_sdm_chan);
    sdm_channel_enable(s_sdm_chan);
}

void alarm_led_set_level(uint8_t percent) {
    if (percent > 100) percent = 100;
    int8_t density = (int8_t)((percent * 255 / 100) - 128);
    sdm_channel_set_pulse_density(s_sdm_chan, density);
}
```

**Оновіть `alarm_driver_init()` (Крок 5), додайте виклик `flow_led_sdm_init()` у кінці.**

> **🟣 РОЗБІР: чому саме так**
> Ідентична техніка й формула RC-фільтра (10 кОм/100 нФ, `f ≈ 159 Гц`), що й
> у Проєкті 6 — саме тому цей компонент виглядає майже дослівно однаково: це
> й демонструє тезу курсу, що "число → напруга" (Модуль 3.4) — універсальний
> примітив, байдужий до того, що саме він показує (загроза чи темп потоку).

> **🔵 ЩО СПОСТЕРІГАЄМО**
> `alarm_led_set_level(100)` при цільовому темпі — LED яскраво світиться;
> при зупинці потоку — гасне.

> **🟠 МІНІ-ЗАВДАННЯ**
> Якби замість LED до вузла після фільтра підключили стрілочний
> мілівольтметр — яку максимальну напругу він показав би при
> `alarm_led_set_level(100)` (підказка: `density=127` не дає рівно 100%
> часу "1", тому напруга буде трохи нижчою за 3.3В)?

---

## 08 · Крок 7 · Серво-затискач через LEDC, безпечний стан ⏱ 40 хв

> **🟢 МЕТА**
> Керувати затискачем через `ledc_set_duty()` (ідентично Кроку 7 Проєкту 6),
> і — найважливіше в цьому кроці — гарантувати, що **за замовчуванням і при
> будь-якій відмові система закриває потік, а не залишає його відкритим**.

**`components/clamp_servo/include/clamp_servo.h`:**
```c
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void clamp_servo_init(void);            // старт ЗАВЖДИ в положенні "закрито"
void clamp_servo_set_open_percent(uint8_t percent);  // 0 = повністю закрито, 100 = повністю відкрито

#ifdef __cplusplus
}
#endif
```

**`components/clamp_servo/clamp_servo.c`:**
```c
#include "clamp_servo.h"
#include "driver/ledc.h"

#define SERVO_PIN       GPIO_NUM_15
#define SERVO_TIMER     LEDC_TIMER_1
#define SERVO_MODE      LEDC_LOW_SPEED_MODE
#define SERVO_CHANNEL   LEDC_CHANNEL_1
#define SERVO_FREQ_HZ   50
#define SERVO_RES       LEDC_TIMER_14_BIT   // апаратна межа ESP32-S3

#define SERVO_MIN_US    500.0f    // 0% відкриття (затискач закритий)
#define SERVO_MAX_US    2500.0f   // 100% відкриття (затискач повністю відкритий)
#define SERVO_PERIOD_US 20000.0f

static void servo_write_pulse(float pulse_us) {
    uint32_t max_duty = (1u << SERVO_RES) - 1u;
    uint32_t duty = (uint32_t)((pulse_us / SERVO_PERIOD_US) * max_duty);
    ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(SERVO_MODE, SERVO_CHANNEL);
}

void clamp_servo_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = SERVO_MODE, .duty_resolution = SERVO_RES,
        .timer_num = SERVO_TIMER, .freq_hz = SERVO_FREQ_HZ, .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .gpio_num = SERVO_PIN, .speed_mode = SERVO_MODE,
        .channel = SERVO_CHANNEL, .timer_sel = SERVO_TIMER,
        .duty = 0, .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);

    servo_write_pulse(SERVO_MIN_US);   // БЕЗПЕЧНИЙ СТАН ЗА ЗАМОВЧУВАННЯМ: закрито
}

void clamp_servo_set_open_percent(uint8_t percent) {
    if (percent > 100) percent = 100;
    float pulse_us = SERVO_MIN_US + (percent / 100.0f) * (SERVO_MAX_US - SERVO_MIN_US);
    servo_write_pulse(pulse_us);
}
```

**`components/clamp_servo/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "clamp_servo.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`clamp_servo_init()` пише `SERVO_MIN_US` (закрито) одразу, до того, як
>   будь-яка бізнес-логіка встигла щось вирішити** — якщо плата
>   перезавантажиться посеред роботи (збій живлення, watchdog), затискач
>   стартує в безпечному положенні, а не в тому, яке було до збою.
> - **Немає жодного шляху коду, де затискач відкривається "за замовчуванням"**
>   — відкриття (Крок 9) завжди явний виклик з конкретним відсотком,
>   вирахуваним з живих показів датчиків; немає стану, де "нічого не
>   вирішено" виглядало б як "відкрито".
> - **`percent`, а не "градуси" в публічному API** — компонент говорить
>   мовою застосунку ("наскільки відкрито"), а перетворення в мікросекунди
>   ізольоване всередині, так само як у Кроці 7 Проєкту 6 з кутом.

> **🔵 ЩО СПОСТЕРІГАЄМО**
> Одразу після прошивки (до будь-якого коду Кроку 9) затискач стоїть у
> положенні "закрито" — переконайтесь у цьому фізично, перш ніж рухатись
> далі.

> **🟠 МІНІ-ЗАВДАННЯ**
> Що б змінилося, якби `clamp_servo_init()` викликав
> `clamp_servo_set_open_percent(100)` замість запису `SERVO_MIN_US`
> напряму? Чому в медичному контексті це небезпечна навіть тимчасова
> поведінка, навіть на кілька секунд до першого реального показу датчика?

---

## 09 · Крок 8 · PCNT-енкодер: набір цільового темпу ⏱ 40 хв

> **🟢 МЕТА**
> Ідентично Кроку 8 Проєкту 6 (PCNT, X4-декодування, коректна протилежна
> полярність каналів) — тепер диск задає цільовий темп (крапель/хв), а не
> кут.

**`components/rate_dial/include/rate_dial.h`:**
```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void  rate_dial_init(void);
float rate_dial_get_target_dpm(void);   // 5..80 крапель/хв
bool  rate_dial_button_pressed(void);   // "озвучити тривогу / тиша"

#ifdef __cplusplus
}
#endif
```

**`components/rate_dial/rate_dial.c`:**
```c
#include "rate_dial.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"

#define DIAL_CLK_PIN   GPIO_NUM_6
#define DIAL_DT_PIN    GPIO_NUM_7
#define DIAL_SW_PIN    GPIO_NUM_8

#define PCNT_LOW_LIMIT   -32768
#define PCNT_HIGH_LIMIT   32767
#define TICKS_PER_DETENT  4
#define DPM_MIN   5.0f
#define DPM_MAX   80.0f
#define DPM_STEP  1.0f   // крапель/хв за один клік диска

static pcnt_unit_handle_t s_pcnt_unit;

void rate_dial_init(void) {
    pcnt_unit_config_t unit_cfg = { .low_limit = PCNT_LOW_LIMIT, .high_limit = PCNT_HIGH_LIMIT };
    pcnt_new_unit(&unit_cfg, &s_pcnt_unit);

    pcnt_glitch_filter_config_t filter_cfg = { .max_glitch_ns = 1000 };
    pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_cfg);

    pcnt_chan_config_t chan_a_cfg = { .edge_gpio_num = DIAL_CLK_PIN, .level_gpio_num = DIAL_DT_PIN };
    pcnt_channel_handle_t chan_a;
    pcnt_new_channel(&s_pcnt_unit, &chan_a_cfg, &chan_a);

    pcnt_chan_config_t chan_b_cfg = { .edge_gpio_num = DIAL_DT_PIN, .level_gpio_num = DIAL_CLK_PIN };
    pcnt_channel_handle_t chan_b;
    pcnt_new_channel(&s_pcnt_unit, &chan_b_cfg, &chan_b);

    // Протилежна полярність каналів A/B — справжнє X4-декодування (Крок 8, Проєкт 6)
    pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_add_watch_point(s_pcnt_unit, PCNT_HIGH_LIMIT);
    pcnt_unit_add_watch_point(s_pcnt_unit, PCNT_LOW_LIMIT);
    pcnt_unit_enable(s_pcnt_unit);
    pcnt_unit_clear_count(s_pcnt_unit);
    pcnt_unit_start(s_pcnt_unit);

    gpio_config_t sw_cfg = {
        .pin_bit_mask = (1ULL << DIAL_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sw_cfg);
}

float rate_dial_get_target_dpm(void) {
    int count = 0;
    pcnt_unit_get_count(s_pcnt_unit, &count);
    float clicks = (float)count / TICKS_PER_DETENT;
    float dpm = 20.0f + clicks * DPM_STEP;   // стартове значення за замовчуванням: 20 крапель/хв
    if (dpm < DPM_MIN) dpm = DPM_MIN;
    if (dpm > DPM_MAX) dpm = DPM_MAX;
    return dpm;
}

bool rate_dial_button_pressed(void) {
    return gpio_get_level(DIAL_SW_PIN) == 0;
}
```

**`components/rate_dial/CMakeLists.txt`:**
```cmake
idf_component_register(
    SRCS "rate_dial.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
```

> **🟣 РОЗБІР: чому саме так**
> - **`DPM_MIN`/`DPM_MAX` обмежують діапазон на рівні читання, а не десь у
>   `main.c`** — компонент сам гарантує, що ніколи не поверне "від'ємний"
>   чи "нескінченний" темп, незалежно від того, скільки обертів фізично
>   зробив оператор.
> - **Стартове значення 20 крапель/хв "зашите" у формулу (`20.0f + ...`),
>   а не збережений стан** — після кожного перезавантаження плата завжди
>   починає з того самого, передбачуваного за замовчуванням темпу, а не з
>   випадкового значення лічильника PCNT.

> **🔵 ЩО СПОСТЕРІГАЄМО**
> Крутіть диск — цільовий темп змінюється кроками по 1 крапель/хв, в межах
> 5-80.

> **🟠 МІНІ-ЗАВДАННЯ**
> Чому саме тут (на відміну від Проєкту 6, де рецентрування енкодера
> обнуляло лічильник) `rate_dial` НЕ викликає `pcnt_unit_clear_count()`
> ніколи? Що сталося б із `dpm`, якби лічильник періодично обнулявся?

---

## 10 · Крок 9 · Інтеграція: контур керування темпом ⏱ 35 хв

> **🟢 МЕТА**
> Замкнути контур: цільовий темп (диск) проти фактичного темпу (краплемір)
> визначає ступінь відкриття затискача й рівень тривоги; відмова датчика чи
> зупинка потоку — безумовне закриття.

**`main/app_main.c` (фінальна версія):**
```c
#include "drop_sensor.h"
#include "temp_sensor.h"
#include "rate_dial.h"
#include "clamp_servo.h"
#include "alarm_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "drip_monitor";

#define STALL_TIMEOUT_MS   8000   // немає крапель довше — вважаємо потік зупиненим

void app_main(void) {
    drop_sensor_init();
    temp_sensor_init();
    rate_dial_init();
    clamp_servo_init();    // старт: затискач ЗАКРИТИЙ (Крок 7)
    alarm_driver_init();

    uint8_t open_percent = 30;   // початкове положення відкриття, скориговане контуром нижче
    bool silenced = false;

    while (1) {
        if (drop_sensor_poll_event()) {
            drop_sensor_register_event();
        }

        float actual_dpm = drop_sensor_get_rate_dpm();
        float target_dpm = rate_dial_get_target_dpm();
        bool stalled = drop_sensor_is_stalled(STALL_TIMEOUT_MS);

        float temp_c, temp_err_c;
        bool temp_ok = temp_sensor_read_celsius(&temp_c, &temp_err_c);

        // --- Керування затискачем: просте пропорційне підстроювання ---
        if (stalled || !temp_ok) {
            open_percent = 0;   // БЕЗУМОВНЕ закриття при відмові будь-якого критичного датчика
        } else {
            float error_dpm = target_dpm - actual_dpm;
            open_percent += (uint8_t)(error_dpm > 0 ? 1 : (error_dpm < 0 ? -1 : 0));
            if (open_percent > 100) open_percent = 100;
            if (open_percent < 0)   open_percent = 0;
        }
        clamp_servo_set_open_percent(open_percent);

        // --- Тривога: ступінчаста за відхиленням від цілі ---
        float deviation_pct = (target_dpm > 0)
            ? 100.0f * (target_dpm - actual_dpm) / target_dpm : 0;
        if (deviation_pct < 0) deviation_pct = -deviation_pct;

        if (stalled || !temp_ok) {
            alarm_buzzer_set(2500, silenced ? 0 : 100);   // максимальна тривога
            alarm_led_set_level(100);
        } else if (deviation_pct > 30.0f) {
            alarm_buzzer_set(1500, silenced ? 0 : 60);
            alarm_led_set_level((uint8_t)(actual_dpm * 100 / 80));
        } else {
            alarm_buzzer_set(1000, 0);   // в межах норми — тиша
            alarm_led_set_level((uint8_t)(actual_dpm * 100 / 80));
        }

        // --- Кнопка диска: тимчасово вимкнути звук (не саму тривогу — LED лишається) ---
        static bool sw_prev = false;
        bool sw_now = rate_dial_button_pressed();
        if (sw_now && !sw_prev) {
            silenced = !silenced;
            ESP_LOGW(TAG, "звук тривоги: %s", silenced ? "вимкнено вручну" : "увімкнено");
        }
        sw_prev = sw_now;

        ESP_LOGI(TAG, "темп=%.1f/%.1f крапель/хв | t=%s%.1f°C | відкриття=%u%% | %s",
                 actual_dpm, target_dpm,
                 temp_ok ? "" : "ПОМИЛКА ", temp_ok ? temp_c : 0.0f,
                 open_percent, stalled ? "ЗУПИНКА ПОТОКУ" : "ОК");

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
```

> **🟣 РОЗБІР: чому саме так**
> - **`stalled || !temp_ok` перевіряється ПЕРШИМ і безумовно закриває
>   затискач** — жодна гілка пропорційного керування нижче не може
>   "переграти" це рішення; відмова датчика завжди виграє над спробою
>   утримати заданий темп.
> - **Кнопка диска вимикає лише звук (`silenced`), а не саму тривогу** —
>   LED-індикатор (Крок 6) продовжує сигналізувати навіть у "тихому"
>   режимі. Це свідома паралель до принципу з Проєкту 5: критичний сигнал
>   не можна повністю приховати одним натисканням, лише зменшити його
>   гучність.
> - **Пропорційне підстроювання `open_percent += ±1` на кожному тіку (20
>   мс)** — навмисно проста (не PID) реакція: якщо фактичний темп нижчий за
>   цільовий, затискач поступово відкривається; якщо вищий — закривається.
>   Повний PID-регулятор — Челендж 3.

> **🔵 ЩО СПОСТЕРІГАЄМО**
> Імітуйте краплі пальцем рідше за цільовий темп — затискач поступово
> відкривається (LED яскравішає). Перестаньте імітувати краплі зовсім на
> 8+ секунд — затискач іде на 0%, лунає максимальна тривога; натисніть SW —
> звук зникає, LED і надалі на максимумі.

> **🟠 МІНІ-ЗАВДАННЯ**
> Чому `open_percent` — змінна типу `uint8_t`, а приріст `error_dpm > 0 ? 1 : -1`
> для `uint8_t` потребує обережності? Що станеться, якщо `open_percent = 0`
> і виконати `open_percent -= 1` без перевірки (підказка: беззнакове
> переповнення)? Перевірте, чи є в коді захист від цього і де саме.

---

## 11 · Крок 10 · Просте тестування ⏱ 15 хв

**Термістор (температура)**
- Торкніться пальцями NTC на кілька секунд → температура в лозі повільно
  зростає; приберіть руку → повільно повертається до кімнатної.
- Витягніть провід NTC (імітація обриву) → в лозі "ПОМИЛКА", затискач
  негайно закривається, лунає максимальна тривога.

**Краплемір (LDR)**
- Рухайте пальцем біля LDR у ритмі, близькому до заданого диском темпу →
  тихо, LED на середній яскравості, затискач стабілізується.
- Рухайте значно рідше цільового темпу → затискач поступово відкривається,
  лунає помірна тривога.
- Не рухайте пальцем 8+ секунд → "ЗУПИНКА ПОТОКУ", затискач на 0%,
  максимальна тривога.

**Диск (цільовий темп)**
- Крутіть — цільовий темп у лозі змінюється кроками.
- Натисніть SW під час тривоги → звук зникає, LED лишається на максимумі.

**Підсумок одним реченням:** будь-яка невизначеність (немає даних, немає
крапель) система тлумачить на користь безпеки — закрито й гучно, а не
відкрито й тихо.

---

## 12 · Челенджі ⏱ ~1 год

### Челендж 1 · Плавний перехід гучності ⏱ 15 хв
Замініть три дискретні рівні гучності (0/60/100%) на безперервну шкалу,
пропорційну `deviation_pct`, використовуючи ту саму формулу лінійного
масштабування, що й `alarm_led_set_level()`.

### Челендж 2 · Точний бюджет похибки термістора ⏱ 20 хв
Порахуйте похідну `dT/dR` формули B-параметра в робочій точці 37°C
(аналог "температури тіла") і виведіть реальний `out_err_c` замість
константи 0.3°C — порівняйте результат із похибкою при 20°C.

### Челендж 3 · Справжній PID замість `±1` ⏱ 20 хв
Замініть пропорційне підстроювання `open_percent += ±1` на повноцінний
PID-регулятор (`Kp`, `Ki`, `Kd`) і порівняйте швидкість виходу на цільовий
темп та коливання навколо нього.

### Челендж 4 · Друга кнопка "тест системи" ⏱ 10 хв
Додайте режим самоперевірки: коротке відкриття затискача на 100% на 2
секунди при старті (до першого реального вимірювання), щоб підтвердити
механічну справність серво, — з чітким логом "самоперевірка ОК/НЕ ОК".

### Челендж 5 · Журнал подій тривоги ⏱ 15 хв
Ведіть у пам'яті (статичний кільцевий буфер, без `malloc`) останні 10 подій
зміни рівня тривоги з часовою міткою (`esp_timer_get_time()`) — і виводьте
його одним блоком за командою (наприклад, довге утримання SW).

---

## ✅ ПІДСУМОК: ЩО МАЄТЕ ОТРИМАТИ В КІНЦІ

- [ ] Модульний ESP-IDF-проєкт (`drip_monitor`) із п'ятьма компонентами,
      об'єднаними лише через `main/app_main.c`.
- [ ] Лічильник крапель на порозі з гістерезисом — і розуміння, чому один
      поріг недостатній для аналогового сигналу з шумом (Модуль 3.1).
- [ ] Відкалібровану температуру (B-параметрична формула) і темп крапель
      (ковзне середнє) з явним бюджетом похибки для обох (Модуль 3.2).
- [ ] Серво-затискач із гарантованим безпечним станом за замовчуванням і
      при будь-якій відмові датчика (Модуль 3.5).
- [ ] SDM-псевдо-ЦАП-індикатор темпу потоку (Модуль 3.4).
- [ ] Ступінчасту ШІМ-тривогу, яку можна тимчасово заглушити, не
      приховуючи сам факт проблеми (Модуль 3.3).
- [ ] PCNT-набір цільового темпу диском без потреби скидати лічильник
      (Модуль 3.6).
- [ ] Ви можете пояснити словами, чому в цьому проєкті "невизначеність
      завжди трактується на користь закритого затискача", і навести ще один
      приклад системи (не медичної), де той самий принцип був би доречний.

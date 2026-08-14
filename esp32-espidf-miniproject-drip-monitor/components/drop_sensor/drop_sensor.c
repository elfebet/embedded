#include "drop_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

static adc_oneshot_unit_handle_t s_adc_handle;

// Гістерезис: крапля, що перетнула світловий промінь, дає ПРОВАЛ напруги
// (менше світла на LDR -> більший опір -> менша напруга на дільнику, якщо
// LDR у верхньому плечі). Два різні пороги замість одного не дають сигналу
// "деренчати" навколо межі.
#define THRESH_LOW  1500 // нижче цього — "крапля в промені" (подія готується)
#define THRESH_HIGH 2000 // вище цього — "промінь чистий" (подія завершена, готові до наступної)

#define RATE_WINDOW_N 5 // ковзне середнє по 5 останніх інтервалах — менш "нервова" оцінка

static bool s_armed = true; // true = чекаємо на провал (початок нової краплі)
static int64_t s_last_event_us = 0;
static float s_intervals_s[RATE_WINDOW_N] = {0};
static int s_interval_idx = 0;
static int64_t s_last_seen_event_us = 0; // для drop_sensor_is_stalled()

void drop_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
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
        s_armed = false; // подія зафіксована, чекаємо повернення вище THRESH_HIGH
        return true;
    }
    if (!s_armed && raw > THRESH_HIGH) {
        s_armed = true; // промінь знову чистий — готові рахувати наступну краплю
    }
    return false;
}

// Викликати ЩОРАЗУ, коли drop_sensor_poll_event() повернув true (Крок 9)
void drop_sensor_register_event(void) {
    int64_t now_us = esp_timer_get_time();
    s_last_seen_event_us = now_us;
    if (s_last_event_us != 0) {
        float dt_s = (now_us - s_last_event_us) / 1e6f;
        if (dt_s > 0.05f && dt_s < 10.0f) { // відкидаємо нефізичні викиди (< 50 мс, > 10 с)
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
        if (s_intervals_s[i] > 0) {
            sum += s_intervals_s[i];
            n++;
        }
    }

    if (n == 0) return 0.0f;

    float avg_interval_s = sum / n;
    return 60.0f / avg_interval_s; // крапель за хвилину
}

bool drop_sensor_is_stalled(uint32_t timeout_ms) {
    if (s_last_seen_event_us == 0) return false; // ще жодної краплі — не "зупинка", а "старт"
    int64_t elapsed_ms = (esp_timer_get_time() - s_last_seen_event_us) / 1000;
    return elapsed_ms > (int64_t)timeout_ms;
}

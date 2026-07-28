#include "include/button_fsm.h"
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "portmacro.h"

#define BTN_DEBONCE_MS 50u      // Stability before PRESSED->HELD & HELD->RELEASE->IDLE
#define BTN_LONG_PRESS_MS 2000u // Threshold SHORT/LONG clicking

static const char *TAG = "panic_isr";
static QueueHandle_t s_panic_queue = NULL;

static void IRAM_ATTR panic_isr_handler(void *arg) {
    uint32_t dummy = 1;
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(s_panic_queue, &dummy, &woken);
    if (woken) portYIELD_FROM_ISR();
}

static void panic_task(void *arg) {
    void (*callback)(void) = (void (*)(void))arg;
    uint32_t dummy;
    while (1) {
        if (xQueueReceive(s_panic_queue, &dummy, portMAX_DELAY)) {
            ESP_LOGE(TAG, "!!! PANIC - hardware of ISR GPIO%d !!!", PANIC_BTN_PIN);
            callback();
        }
    }
}

void panic_sensor_init(button_type_t type, void (*on_panic_deferred)(void)) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PANIC_BTN_PIN),
        .mode = GPIO_MODE_INPUT,
    };
    if (type == BTN_PULLDOWN) {
        cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.intr_type = GPIO_INTR_POSEDGE;
    } else {
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        cfg.intr_type = GPIO_INTR_NEGEDGE;
    }

    gpio_config(&cfg);

    s_panic_queue = xQueueCreate(4, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PANIC_BTN_PIN, panic_isr_handler, NULL);

    xTaskCreate(panic_task, "panic_task", 3072, (void *)on_panic_deferred, 15, NULL);
}

void button_fsm_init(button_fsm_t *btn, gpio_num_t pin, button_type_t type) {
    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = (type == BTN_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .pull_up_en = (type == BTN_PULLDOWN) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    btn->pin = pin;
    btn->type = type;
    btn->state = BTN_STATE_IDLE;
    btn->state_enter_ms = 0;
    btn->long_fired = false;
}

bool is_button_pressed(button_fsm_t *btn) {
    int level = gpio_get_level(btn->pin);
    if (btn->type == BTN_PULLDOWN) {
        return level == 1;
    } else {
        return level == 0;
    }
}

button_event_t button_fsm_tick(button_fsm_t *btn, uint32_t now_ms) {
    bool pressed = is_button_pressed(btn);
    button_event_t event = BTN_EVENT_NONE;

    switch (btn->state) {
        case BTN_STATE_IDLE:
            if (pressed) {
                btn->state = BTN_STATE_PRESSED;
                btn->state_enter_ms = now_ms;
            }
            break;
        case BTN_STATE_PRESSED:
            if (!pressed) {
                btn->state = BTN_STATE_IDLE;
            } else if (now_ms - btn->state_enter_ms >= BTN_DEBONCE_MS) {
                btn->state = BTN_STATE_HELD;
                btn->state_enter_ms = now_ms;
                btn->long_fired = false;
            }
            break;
        case BTN_STATE_HELD: {
            uint32_t held_ms = now_ms - btn->state_enter_ms;
            if (!pressed) {
                if (!btn->long_fired)
                    event = BTN_EVENT_SHORT_PRESS;
                btn->state = BTN_STATE_RELEASE;
                btn->state_enter_ms = now_ms;
            } else if (!btn->long_fired && held_ms >= BTN_LONG_PRESS_MS) {
                event = BTN_EVENT_LONG_PRESS;
                btn->long_fired = true;
            }
            break;
        }
        case BTN_STATE_RELEASE:
            if (pressed) {
                btn->state = BTN_STATE_HELD;
            } else if (now_ms - btn->state_enter_ms >= BTN_DEBONCE_MS) {
                btn->state = BTN_STATE_IDLE;
            }
            break;
    }
    return event;
}
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "button_fsm.h"
#include "load_driver.h"
#include "pot_sensor.h"
#include "nvs_storage.h"
#include "freertos/projdefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "arming_console";
static QueueHandle_t s_chirp_queue = NULL;

static void request_chirp(uint32_t ms) {
    xQueueSend(s_chirp_queue, &ms, 0);
}

static void buzzer_task(void *arg) {
    uint32_t ms;
    while (1) {
        if (xQueueReceive(s_chirp_queue, &ms, portMAX_DELAY)) {
            load_buzzer_chirp(ms);
        }
    }
}

// FSM is all of SECURITY SYSTEM OF PANEL
typedef enum {
    SYS_DISARMED = 0, // система знята з охорони, тестування зон дозволене без тривоги
    SYS_ARMING,       // йде відлік затримки постановки (потенціометр задає тривалість)
    SYS_ARMED,        // на охороні: спрацювання будь-якої зони = тривога
    SYS_ALARM,        // тривога активна: сирена, доки не роззброять (BTN1 коротко)
} system_state_t;

typedef struct {
    const char *name;
    bool bypassed; // long pressed of button -> turn off of control
    bool tripped;  // worked (for logs/diagnostics)
} zone_t;

#define N_ZONES 2
static zone_t s_zones[N_ZONES] = {
	{ .name = "ZONE1", .bypassed = false, .tripped = false },
	{ .name = "ZONE2", .bypassed = false, .tripped = false },
};

// volatile: writing of cross-context (ISR-task PANIC) -> reading of main-loop
static volatile bool s_panic_latched = false;
static system_state_t s_sys_state = SYS_DISARMED;
static uint32_t s_state_enter_ms = 0;
static uint32_t s_arming_total_s = 0; // how long will happen ARMING

#define N_FSM_BUTTONS 3
enum {
    IDX_ARM = 0,
    IDX_ZONE1,
    IDX_ZONE2
};
static button_fsm_t s_buttons[N_FSM_BUTTONS];

#define ARM_DELAY_MIN_S 3u
#define ARM_DELAY_MAX_S 15u

static uint32_t arming_delay_seconds(void) {
    uint16_t raw = pot_sensor_get_raw();
    return ARM_DELAY_MIN_S + ((uint32_t)raw * (ARM_DELAY_MAX_S - ARM_DELAY_MIN_S)) / 4095u;
};

static void sys_transition(system_state_t new_state, uint32_t now_ms) {
    ESP_LOGW(TAG, "SYSTEM: %d -> %d", s_sys_state, new_state);
    s_sys_state = new_state;
    s_state_enter_ms = now_ms;
}

// --- Handlers of events buttons of FSM
static void handle_arm_button(button_event_t ev, uint32_t now_ms) {
    if (ev == BTN_EVENT_SHORT_PRESS) {
        switch (s_sys_state) {
            case SYS_DISARMED:
                s_arming_total_s = arming_delay_seconds();
                sys_transition(SYS_ARMING, now_ms);
                request_chirp(60);
                ESP_LOGI(TAG, "Installation of security: %lu s to ARMED", (unsigned long)s_arming_total_s);
                break;

            case SYS_ARMING:
                sys_transition(SYS_DISARMED, now_ms);
                request_chirp(200);
                break;

            case SYS_ARMED:
                sys_transition(SYS_DISARMED, now_ms);
                request_chirp(300);
                ESP_LOGI(TAG, "Disarmed.");
                break;

            case SYS_ALARM:
                sys_transition(SYS_DISARMED, now_ms);
                load_buzzer_set(false);
                s_panic_latched = false;
                ESP_LOGW(TAG, "The alarm has been cleared, the system disarmed.");
                break;
        }
    } else if (ev == BTN_EVENT_LONG_PRESS) {
        for (int i = 0; i < N_ZONES; i++) {
            s_zones[i].bypassed = false;
            s_zones[i].tripped = false;
        }
        s_panic_latched = false;
        nvs_storage_erase();
        sys_transition(SYS_DISARMED, now_ms);
        load_buzzer_set(false);
        ESP_LOGW(TAG, "FULL RESET OFF PANEL (LONG PRESS ARM).");
    }
}

static void handle_zone_button(int zone_idx, button_event_t ev) {
    zone_t *z = &s_zones[zone_idx];
    if (ev == BTN_EVENT_SHORT_PRESS) {
        z->tripped = true;
        nvs_storage_write_bool(z->name, STORAGE_VALUE_TRIPPED, z->tripped);
        ESP_LOGI(TAG, "%s: simulation working of sensor (bypass=%d)", z->name, z->bypassed);
        if (s_sys_state == SYS_ARMED && !z->bypassed) {
            sys_transition(SYS_ALARM, (uint32_t)(esp_timer_get_time() / 1000ULL));
            ESP_LOGE(TAG, "!!! ALARM: %s !!!", z->name);
        }
    } else if (ev == BTN_EVENT_LONG_PRESS) {
        z->bypassed = !z->bypassed;
        nvs_storage_write_bool(z->name, STORAGE_VALUE_BYPASSED, z->bypassed);
        request_chirp(z->bypassed ? 150: 50);
        ESP_LOGW(TAG, "%s: bypass %s", z->name, z->bypassed ? "TURN ON" : "TURN OFF");
    }
}

void button_fsm_timer_tick(void *args) {
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    button_event_t ev;

    ev = button_fsm_tick(&s_buttons[IDX_ARM], now_ms);
    if (ev != BTN_EVENT_NONE) handle_arm_button(ev, now_ms);

    ev = button_fsm_tick(&s_buttons[IDX_ZONE1], now_ms);
    if (ev != BTN_EVENT_NONE) handle_zone_button(0, ev);

    ev = button_fsm_tick(&s_buttons[IDX_ZONE2], now_ms);
    if (ev != BTN_EVENT_NONE) handle_zone_button(1, ev);

    // --- System transition ARMING -> ARMED for time (button not event, but timeout) ---
    if (s_sys_state == SYS_ARMING) {
        uint32_t elapsed_s = (now_ms - s_state_enter_ms) / 1000u;
        if (elapsed_s >= s_arming_total_s) {
            sys_transition(SYS_ARMED, now_ms);
            request_chirp(500);
            ESP_LOGW(TAG, "The system is armed.");
        }
    }

    // --- PANIC has the highest priority and "overrides"" the system in ANY state ---
    if (s_panic_latched && s_sys_state != SYS_ALARM) {
        sys_transition(SYS_ALARM, now_ms);
    }

    // --- Keep ALARM state actions active until reset (handle arm button) ---
    if (s_sys_state == SYS_ALARM) {
        load_buzzer_set(true);
    }
}

void start_button_fsm_timer(void) {
    const esp_timer_create_args_t args = {
        .callback = &button_fsm_timer_tick,
        .name = "fsm_tick"
    };
    esp_timer_handle_t timer;
    esp_timer_create(&args, &timer);
    esp_timer_start_periodic(timer, 10u  * 1000ULL); // esp_timer to count to microseconds
}

// Handling the PANIC interrupt (Step 6) — executed in panic_task, NOT in an ISR
static void on_panic_deferred(void) {
    s_panic_latched = true; // The actual FSM state change will occur in app_fsm_tick_all (next tick, ≤10 ms).
    ESP_LOGE(TAG, "PANIC detected; an alarm will be raised on the next FSM tick.");
}

static void arming_beep_task(void *arg) {
  while (1) {
    if (s_sys_state == SYS_ARMING) {
      uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
      uint32_t elapsed_s = (now_ms - s_state_enter_ms) / 1000u;
      uint32_t remaining_s = (elapsed_s < s_arming_total_s) ? (s_arming_total_s - elapsed_s) : 0;
      // the less time remaining, the more frequent the beeping: 500 ms (calm) .. 80 ms (urgent)
      uint32_t beep_period_ms = 80 + (remaining_s * 420) / (ARM_DELAY_MAX_S);
      load_buzzer_set(true);
      vTaskDelay(pdMS_TO_TICKS(30)); // a short "tick," not a long signal
      load_buzzer_set(false);
      vTaskDelay(pdMS_TO_TICKS(beep_period_ms));
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));  // не ARMING — просто чекаємо, нічого не робимо
    }
  }
}

const char *sys_state_name() {
    switch (s_sys_state) {
        case SYS_ARMING:
            return "ARMING";
        case SYS_ARMED:
            return "ARMED";
        case SYS_ALARM:
            return "ALARM";
        case SYS_DISARMED:
        default:
            return "DISARMED";
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Start app main");
    
    // setup load driver
    load_driver_init();
    load_buzzer_set_volume(0);

    // create NVS storage and read "bypassed" values
    nvs_storage_init();
    for (int i = 0; i < N_ZONES; i++) {
        s_zones[i].bypassed = nvs_storage_read_bool(s_zones[i].name, STORAGE_VALUE_BYPASSED);
        s_zones[i].tripped = nvs_storage_read_bool(s_zones[i].name, STORAGE_VALUE_TRIPPED);
    }

    // init buttons
    button_fsm_init(&s_buttons[IDX_ARM], GPIO_NUM_4, BTN_PULLDOWN);
    button_fsm_init(&s_buttons[IDX_ZONE1], GPIO_NUM_5, BTN_PULLDOWN);
    button_fsm_init(&s_buttons[IDX_ZONE2], GPIO_NUM_6, BTN_PULLUP);
    panic_sensor_init(BTN_PULLUP, on_panic_deferred);
    
    // setup potentiometer 
    pot_sensor_init();

    s_chirp_queue = xQueueCreate(4, sizeof(uint32_t));
    xTaskCreate(buzzer_task, "buzzer_task", 3072, NULL, 4, NULL);

    start_button_fsm_timer();

    xTaskCreate(arming_beep_task, "arming_beep", 3072, NULL, 4, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "STATE=%s\tZONE1(bypass=%d, trip=%d)\tZONE2(bypass=%d, trip=%d)\t[POT=%u]",
                 sys_state_name(), 
                 s_zones[0].bypassed, s_zones[0].tripped, 
                 s_zones[1].bypassed, s_zones[1].tripped,
                 pot_sensor_get_raw());
    }
}

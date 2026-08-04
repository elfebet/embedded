#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "soc/gpio_num.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "note.h"

#define BTN_GPIO            GPIO_NUM_6
#define BUZZER_GPIO         GPIO_NUM_5
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_TIMER_RES      LEDC_TIMER_10_BIT

static const char *TAG = "BUZZER_SOUND";

static const Note *current_song = NULL;
static Melody current_melody = MELODY_JINGLE_BELLS;
static esp_timer_handle_t music_timer = NULL;

static size_t song_idx = 0;
static uint8_t ticks_remaining = 0;
static bool is_pause_note = false;

// ------- MELODY functions -------

static const Note *notes(Melody melody) {
    switch (melody) {
    case MELODY_JINGLE_BELLS:
        return NOTES_JINGLE_BELLS;
    case MELODY_BABY_SHARK:
        return NOTES_BABY_SHARK;
    case MELODY_TWINKLE:
        return NOTES_TWINKLE;
    case MELODY_WE_WILL_ROCK_YOU:
        return NOTES_WE_WILL_ROCK_YOU;
    default:
        return NOTES_JINGLE_BELLS;
    }
}

static const char *melody_name(Melody melody) {
    switch (melody) {
    case MELODY_JINGLE_BELLS:
        return "JINGLE BELLS";
    case MELODY_BABY_SHARK:
        return "BABY SHARK";
    case MELODY_TWINKLE:
        return "TWINKLE";
    case MELODY_WE_WILL_ROCK_YOU:
        return "WE WILL ROCK YOU";
    default:
        return "Unknown";
    }
}

static Melody next_melody(Melody current) {
    switch (current) {
    case MELODY_JINGLE_BELLS:
        return MELODY_BABY_SHARK;
    case MELODY_BABY_SHARK:
        return MELODY_TWINKLE;
    case MELODY_TWINKLE:
        return MELODY_WE_WILL_ROCK_YOU;
    case MELODY_WE_WILL_ROCK_YOU:
    default:
        return MELODY_JINGLE_BELLS;
    }
}

// ------- PLAYER functions -------

static void set_note_freq(uint16_t freq_hz) {
    if (freq_hz == REST) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    } else {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_hz);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 512); // 50% duty cycle
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    }
}

// timer callback, tick every TICK_PERIOD_MS (50 ms)
static void player_tick_callback(void* arg) {
    if (current_song == NULL) return;

    if (ticks_remaining > 0) {
        ticks_remaining--;
        return;
    }

    // If pause_note between notes -> play next note
    if (is_pause_note) {
        is_pause_note = false;
        const Note *note = &current_song[song_idx];

        if (note->freq == 0 && note->duration == 0) {
            // Finish marker
            set_note_freq(REST);
            current_song = NULL;
            esp_timer_stop(music_timer);
            ESP_LOGI(TAG, "Finish song");
            return;
        }

        set_note_freq(note->freq);
        ticks_remaining = note->duration + 1; // add pause_note "pause-separator"
        song_idx++;
    } else {
        // short pause between notes (1 tick = 50 ms)
        // It needs to play sounds separately, don't blend together 
        set_note_freq(REST);
        is_pause_note = true;
        ticks_remaining = 1; 
    }
}

void player_init(void) {
    const ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_TIMER_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = 1000, // Initial frequency
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    const ledc_channel_config_t channel_conf = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&channel_conf);

    const esp_timer_create_args_t timer_args = {
        .callback = &player_tick_callback,
        .name = "player_timer"
    };
    esp_timer_create(&timer_args, &music_timer);
}

bool is_player_active(void) {
    return esp_timer_is_active(music_timer);
}

void player_start_melody(const Melody melody) {
    if (is_player_active()) {
        esp_timer_stop(music_timer);
    }

    song_idx = 0;
    ticks_remaining = 0;
    is_pause_note = true;
    current_song = notes(melody);

    ESP_LOGI(TAG, "Start play %s", melody_name(melody));
    esp_timer_start_periodic(music_timer, TICK_PERIOD_MS * 1000);
}

// ------- BUTTON functions -------

static QueueHandle_t button_queue = NULL;
static volatile uint64_t button_last_isr_time = 0;

static void IRAM_ATTR button_isr(void *arg) {
    uint64_t now = esp_timer_get_time();
    // debounce delay in microseconds (200 ms)
    if (now - button_last_isr_time > 200000ULL) {
        uint32_t delay = now - button_last_isr_time;
        button_last_isr_time = now;

        BaseType_t higher_priority_task_woken = pdFALSE;
        xQueueSendFromISR(button_queue, &delay, &higher_priority_task_woken); // Send counter to queue from ISR
        if (higher_priority_task_woken) {
            portYIELD_FROM_ISR();
        }
    }
}

void button_pressed(void) {
    if (is_player_active()) {
        ESP_LOGI(TAG, "[Button pressed] Ignore, current song is playing");
        return;
    }

    current_melody = next_melody(current_melody);
    player_start_melody(current_melody);
}

void button_task(void *pvParameters) {
    const gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE // rising
    };
    gpio_config(&btn_conf);

    // create button queue
    button_queue = xQueueCreate(10, sizeof(uint32_t));
    
    // install ISR Service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, button_isr, NULL);

    // keep program running
    uint32_t delay;
    while (1) {
        // Wait indefinitely for an item in the queue
        if (xQueueReceive(button_queue, &delay, portMAX_DELAY)) {
            button_pressed();
        }
    }
}

// ------- MAIN -------

void app_main(void)
{
    // button task
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    player_init();
    player_start_melody(current_melody);
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define BUTTON_PIN GPIO_NUM_4
#define LED_PIN GPIO_NUM_5
#define DEBOUNCE_TIME_MS 50

static const char *TAG = "DEBOUNCE";
static QueueHandle_t button_evt_queue = NULL;
static esp_timer_handle_t debounce_timer = NULL;
static bool led_state = false;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;
    // temporary disable interutps for button, to ignore clang
    gpio_intr_disable(gpio_num);
    // start timer with 50 ms only once
    esp_timer_start_once(debounce_timer, DEBOUNCE_TIME_MS * 1000);
}

static void button_timer_callback(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    int current_level = gpio_get_level(gpio_num);
    if (current_level == 1) { 
        xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
    }
    // enable interrupt for this button gpio
    gpio_intr_enable(gpio_num);
}

static void button_processor_task(void* arg) {
    uint32_t gpio_num;
    uint32_t press_count = 0;
    ESP_LOGI(TAG, "Button task is running");
    
    for(;;) {
        if(xQueueReceive(button_evt_queue, &gpio_num, portMAX_DELAY)) {
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state);
            ESP_LOGI(TAG, "Button pressed, toggle led state: %d", led_state);
        }
    }
}

void app_main(void) {
    // led config
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, led_state);

    // button config (pulldown button)
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE // rising
    };
    gpio_config(&btn_conf);

    // create button queue
    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // confnigure esp timer
    const esp_timer_create_args_t timer_args = {
        .callback = &button_timer_callback,
        .arg = (void*) BUTTON_PIN,
        .name = "debounce_timer"
    };
    esp_timer_create(&timer_args, &debounce_timer);

    // install ISR Service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*)BUTTON_PIN);

    // create button task
    xTaskCreate(button_processor_task, "button_task", 2048, NULL, 10, NULL);
}
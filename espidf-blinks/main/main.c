#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "soc/gpio_num.h"

#define LED_PIN GPIO_NUM_6

static const char *TAG = "blink_tag";

void app_main(void)
{
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	ESP_LOGI(TAG, "Номер піна: %d", LED_PIN);

    while (true) {
		for (int i = 0; i < 3; i++) {
			gpio_set_level(LED_PIN, 1);
			vTaskDelay(200 / portTICK_PERIOD_MS);
			gpio_set_level(LED_PIN, 0);		
			vTaskDelay(200 / portTICK_PERIOD_MS);
		}
		ESP_LOGI(TAG, "Затримка на 3 секунди");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

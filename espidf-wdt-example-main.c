//#include <stdint.h>
#include <stdio.h>
//#include <string.h>
#include <sys/_intsup.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

static const char *TAG = "WDT";

void app_main(void) {
	esp_task_wdt_config_t twdt_config = {
		.timeout_ms = 5000,
		.idle_core_mask = 0,
		.trigger_panic = true,
	};
	
	esp_task_wdt_init(&twdt_config);
	ESP_LOGI(TAG, "%p", &twdt_config);
	
	esp_task_wdt_add(NULL);
	
	uint64_t iter = 0;
    while (true) {
		ESP_LOGI(TAG, "Work... %d", iter);
		iter += 1;
		esp_task_wdt_reset();
		
		if (iter == 10) {
			ESP_LOGW(TAG, "Simulate crash (infinite loop)");
			while (true) {
				// esp_task_wdt_reset();  
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
    }
}

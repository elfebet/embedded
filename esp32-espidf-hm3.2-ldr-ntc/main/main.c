#include "esp_log.h"
#include "ldr.h"
#include "ntc.h"

static const char *TAG = "LDR_and_NTC";

void app_main(void)
{
    ESP_LOGI(TAG, "Start LDR and NTC tasks");
    ldr_setup();
    ntc_setup();
}
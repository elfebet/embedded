#include "include/nvs_storage.h"
#include <stdio.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

static nvs_handle_t my_handle = 0;

bool nvs_storage_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase flash when an error happens and try init again
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (ret == ESP_OK) {
        ret = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (ret != ESP_OK) {
            printf("Error. Unable to open NVS: %s\n", esp_err_to_name(ret));
        }
    }

    return ret == ESP_OK;
}

bool nvs_storage_readBypassedValue(const char *zoneName) {
    uint8_t value = 0;
    esp_err_t ret = nvs_get_u8(my_handle, zoneName, &value);
    printf("Read bypassed value for %s. Result: %s\n", zoneName, esp_err_to_name(ret));
    return value == 1;
}

void nvs_storage_writeBypassedValue(const char *zoneName, bool value) {
    esp_err_t ret = nvs_set_u8(my_handle, zoneName, (uint8_t)value);
    if (ret == ESP_OK) {
        nvs_commit(my_handle);
    } else {
        printf("Error write value to NVS: %s\n", esp_err_to_name(ret));
    }
}

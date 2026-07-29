#include "include/nvs_storage.h"
//#include <cstddef>
#include <stdio.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NVS_KEY_MAX_LEN 15 // 15 characters

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

const char *key_name(const char *zoneName, storage_value_t valueName) {
    static char key_buffer[NVS_KEY_MAX_LEN];
    if (zoneName == NULL) {
        return NULL;
    }

    int written = snprintf(key_buffer, sizeof(key_buffer), "%s_%d", zoneName, (int)valueName);
    if (written < 0 || written >= (int)sizeof(key_buffer)) {
        return NULL; 
    }

    return key_buffer;
}

bool nvs_storage_read_bool(const char *zoneName, storage_value_t valueName) {
    uint8_t value = 0;
    const char *keyName = key_name(zoneName, valueName);
    esp_err_t ret = nvs_get_u8(my_handle, keyName, &value);
    printf("Read key value for %s. Result: %s\n", keyName, esp_err_to_name(ret));
    return value == 1;
}

void nvs_storage_write_bool(const char *zoneName, storage_value_t valueName, bool value) {
    const char *keyName = key_name(zoneName, valueName);
    esp_err_t ret = nvs_set_u8(my_handle, keyName, (uint8_t)value);
    if (ret == ESP_OK) {
        nvs_commit(my_handle);
    } else {
        printf("Error write value to NVS: %s\n", esp_err_to_name(ret));
    }
}

void nvs_storage_erase() {
    esp_err_t err = nvs_erase_all(my_handle);
    if (err == ESP_OK) {
        nvs_commit(my_handle);
    }
}

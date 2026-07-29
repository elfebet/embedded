#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STORAGE_VALUE_BYPASSED = 0,
    STORAGE_VALUE_TRIPPED,
} storage_value_t;

bool nvs_storage_init(); // init and open NVS
bool nvs_storage_read_bool(const char *zoneName, storage_value_t valueName);
void nvs_storage_write_bool(const char *zoneName, storage_value_t valueName, bool value);
void nvs_storage_erase();

#ifdef __cplusplus
}
#endif
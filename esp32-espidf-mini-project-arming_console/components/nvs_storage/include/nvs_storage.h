#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool nvs_storage_init(); // init and open NVS
bool nvs_storage_readBypassedValue(const char *zoneName);
void nvs_storage_writeBypassedValue(const char *zoneName, bool value);

#ifdef __cplusplus
}
#endif
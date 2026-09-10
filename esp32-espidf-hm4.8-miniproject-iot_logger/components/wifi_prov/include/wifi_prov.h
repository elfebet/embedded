#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void wifi_prov_init(void); // 1
bool wifi_prov_is_provisioned(void); // 2
void wifi_prov_connect_to_saved_wifi(void); // 3
void wifi_prov_start_provisioning(void); // 4

#ifdef __cplusplus
}
#endif
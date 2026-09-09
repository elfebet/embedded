#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mqtt_link_led_cb_t)(bool led_on);
typedef void (*mqtt_link_time_synced_cb_t)(void);

// call AFTER wifi_prov_init_and_connect()
// mqtt works for SNTP (Tiny RTC) + connect to broker
void mqtt_link_init(mqtt_link_led_cb_t on_led_command, mqtt_link_time_synced_cb_t on_time_synced);
bool mqtt_link_is_connected(void);
void mqtt_link_publish_telemetry(const char *json_payload);

#ifdef __cplusplus
}
#endif
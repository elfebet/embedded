#include <stdio.h>
#include <sys/_intsup.h>
#include "wifi_prov.h"
#include "network_provisioning/manager.h"
#include "network_provisioning/scheme_softap.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"

// If stored WiFi credentials (in NVS) can't connect to this device during timeout - 
// consider it non-working (credentials outdated), reset provisioning and got to SoftAP again (restart esp32s3)
#define STA_CONNECT_TIMEOUT_MS   30000 // 30 sec

static const char *TAG = "WIFI_PROV";
static EventGroupHandle_t s_event_group;
static const int CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    ESP_LOGD(TAG, "[Event Handler] base: %s, id: %d", base, id);

    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_STA_START) {
            ESP_LOGD(TAG, "Wi-Fi STA is running, waiting commands...");
        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGD(TAG, "Wi-Fi STA disconnected. Connect to wifi again");
            esp_wifi_connect();
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)data;
        ESP_LOGI(TAG, "Successfully connected! IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_event_group, CONNECTED_BIT);

    } else if (base == NETWORK_PROV_EVENT) {
        switch (id) {
        case NETWORK_PROV_INIT:
             ESP_LOGD(TAG, "Network prov initialized.");
             break;
        case NETWORK_PROV_START:
            ESP_LOGD(TAG, "Provisioning is started - connect your phone to Wi-Fi for this device");
            break;
        case NETWORK_PROV_WIFI_CRED_RECV: {
            wifi_sta_config_t *cfg = (wifi_sta_config_t *)data;
            ESP_LOGD(TAG, "Received data: SSID=%s", (const char *)cfg->ssid);
            break;
        }
        case NETWORK_PROV_WIFI_CRED_SUCCESS:
            ESP_LOGD(TAG, "Wi-Fi data successfully saved into NVS!");
            break;
        case NETWORK_PROV_WIFI_CRED_FAIL:
            ESP_LOGE(TAG, "Provisioning failed - please check SSID/password and try again");
            break;
        case NETWORK_PROV_END:
            ESP_LOGD(TAG, "Provisioning finished, deinit provisioning.");
            network_prov_mgr_deinit();
            break;
        case NETWORK_PROV_DEINIT:
            ESP_LOGD(TAG, "Provisioning was deinit");
            break;
        default:
            ESP_LOGD(TAG, "Unknown id: %d", id);
            break;
        }
    }
}

// 1. Initialization
void wifi_prov_init(void) {
    // Init NVS (required to save Wi-Fi data)
    ESP_ERROR_CHECK(nvs_flash_init());

    s_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Registering event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // Init Wi-Fi in default state
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    network_prov_mgr_config_t prov_cfg = {
        .scheme = network_prov_scheme_softap,
        .scheme_event_handler = NETWORK_PROV_EVENT_HANDLER_NONE,
    };
    ESP_ERROR_CHECK(network_prov_mgr_init(prov_cfg));
}

// 2. Check is provisioned
bool wifi_prov_is_provisioned(void) {
    bool provisioned = false;
    ESP_ERROR_CHECK(network_prov_mgr_is_wifi_provisioned(&provisioned));
    return provisioned;
}

// 3. If provisioned==true, connect to saved wifi
void wifi_prov_connect_to_saved_wifi(void) {
    network_prov_mgr_deinit();

    ESP_LOGI(TAG, "Found saved Wi-Fi data. Connecting...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Waiting for IP with stored credentials (timeout %d sec)...", STA_CONNECT_TIMEOUT_MS / 1000);
    EventBits_t bits = xEventGroupWaitBits(
        s_event_group, 
        CONNECTED_BIT, 
        pdFALSE, 
        pdTRUE,
        pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS)
    );
    if (!(bits & CONNECTED_BIT)) {
        ESP_LOGW(TAG, "Stored Wi-Fi credentials did not connect in time -- resetting "
                      "provisioning and rebooting into SoftAP so they can be re-entered");
        network_prov_mgr_reset_wifi_provisioning();
        esp_restart();
    }

    ESP_LOGI(TAG, "Wi-Fi is ready");
}

// 4. If provisioned==false, start provisioning
void wifi_prov_start_provisioning(void) {
    ESP_LOGI(TAG, "Wi-FI data not found. Start Provisioning...");

    esp_netif_create_default_wifi_ap();  // needs only for SoftAP-provisioning

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char service_name[24];
    snprintf(service_name, sizeof(service_name), "PROV_iot_logger_%02X%02X", mac[4], mac[5]);

    ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(
        NETWORK_PROV_SECURITY_1,
        CONFIG_IOT_LOGGER_PROVISIONING_PIN, // pin for enter in app
        service_name, 
        NULL
    ));

    // waiting without timeout (it's normal state). waiting for open app on phone
    ESP_LOGI(TAG, "Waiting for IP...");
    xEventGroupWaitBits(
        s_event_group, 
        CONNECTED_BIT, 
        pdFALSE, 
        pdTRUE, 
        portMAX_DELAY
    );

    ESP_LOGI(TAG, "Wi-Fi is ready");
}

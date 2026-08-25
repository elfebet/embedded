#include "temp_sensor.h"
#include "ds18b20_types.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

static onewire_bus_handle_t s_onewire_bus_handle = NULL;
static ds18b20_device_handle_t s_ds18b20s_handle = NULL;
static float s_temperature = 0;
static const char *TAG = "TEMPERATURE";

void ds18b20_task(void *args) {
    uint8_t pin = (int)args;

    // install 1-wire bus
    onewire_bus_config_t bus_config = { .bus_gpio_num = pin };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &s_onewire_bus_handle));
    ESP_LOGI(TAG, "1-Wire bus installed on GPIO%d", pin);

    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;

    // create 1-wire device iterator, which is used for device search
    ESP_ERROR_CHECK(onewire_new_device_iter(s_onewire_bus_handle, &iter));
    if (onewire_device_iter_get_next(iter, &next_onewire_device) == ESP_OK) {
        ds18b20_config_t ds_cfg = {};
        // check if the device is a DS18B20, if so, return the ds18b20 handle
        if (ds18b20_new_device_from_enumeration(&next_onewire_device, &ds_cfg, &s_ds18b20s_handle) == ESP_OK) {
            onewire_device_address_t address;
            ds18b20_get_device_address(s_ds18b20s_handle, &address);
            ESP_LOGI(TAG, "Found a DS18B20, address: %016llX", address);
        } else {
            ESP_LOGI(TAG, "Found an unknown device, address: %016llX", next_onewire_device.address);
        }
    }
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));

    if (s_ds18b20s_handle == NULL) {
        ESP_LOGE(TAG, "Not found any DS18B20 device");
        vTaskDelete(NULL); // delete self task
        return;
    }

    ESP_LOGI(TAG, "Searching done, DS18B20 device found");
    ds18b20_set_resolution(s_ds18b20s_handle, DS18B20_RESOLUTION_9B);

    float temperature = 0;
    while (1) {
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(s_ds18b20s_handle));
        ESP_ERROR_CHECK(ds18b20_get_temperature(s_ds18b20s_handle, &temperature));
        s_temperature = temperature;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void temp_sensor_task_create(uint8_t pin) {
    xTaskCreate(
        ds18b20_task,
        "ds18b20_task",
        4096,
        (void *)(int)pin,
        5, 
        NULL
    );
}

float temp_sensor_get_temperature() {
    return s_temperature;
}
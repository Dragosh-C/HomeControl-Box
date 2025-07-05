// app_main.c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_setup.h"
#include "mqtt.h"
#include "sensors/sensor.h"

static const char *TAG = "app_main_main_box_1000";

void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // relays_func();
    relays_func();
    // Initialize Wi-Fi
    wifi_init_sta();

    // Initialize MQTT client
    mqtt_start();

    // Read sensor data
    sensor_func();
    
}




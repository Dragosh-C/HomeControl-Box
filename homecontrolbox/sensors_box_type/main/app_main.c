// app_main.c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_setup.h"
#include "mqtt.h"
#include "sensors/sensor.h"

static const char *TAG = "app_main";

void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Wi-Fi
    wifi_init_sta();

    // Initialize MQTT client
    mqtt_start();

    // Read sensor data
    sensor_func();
}




// #include <stdio.h>
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_event.h"
// #include "esp_netif.h"
// #include "esp_sleep.h"
// #include "wifi_setup.h"
// #include "mqtt.h"
// #include "sensors/sensor.h"

// static const char *TAG = "app_main";

// void app_main(void) {
//     ESP_LOGI(TAG, "[APP] Booting...");

//     ESP_ERROR_CHECK(nvs_flash_init());
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     wifi_init_sta();     // Connect to Wi-Fi
//     mqtt_start();        // Start MQTT

//     vTaskDelay(pdMS_TO_TICKS(2000)); // Optional delay to ensure MQTT connects

//     sensor_func();  // Modified function: just send once, no tasks or loops

//     ESP_LOGI(TAG, "Sleeping for 10 seconds...");
//     esp_sleep_enable_timer_wakeup(10 * 1000000); // 10 seconds (µs)
//     esp_deep_sleep_start();
// }


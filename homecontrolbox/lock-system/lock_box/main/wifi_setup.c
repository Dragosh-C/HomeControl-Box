#include <stdio.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
// #include "output/led_signals.h"

static const char *TAG = "wifi_setup_box_4123";



static bool wifi_connected = false;



void verify_wifi_connection(void *param) {
    ESP_LOGI(TAG, "Starting Wi-Fi verification task");

    TickType_t last_check_time = xTaskGetTickCount();
    const TickType_t verify_interval = pdMS_TO_TICKS(10000);

    while (1) {
        // Check if 10 seconds have passed since the last verification
        if (xTaskGetTickCount() - last_check_time >= verify_interval) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                ESP_LOGI(TAG, "Wi-Fi connection verified. SSID: %s", ap_info.ssid);
            } else {
                ESP_LOGE(TAG, "Lost Wi-Fi connection. Restarting LED task.");
                wifi_connected = false;
            }

            last_check_time = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}



void wifi_init_sta() {
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    // Initialize Wi-Fi
    // ESP_ERROR_CHECK(example_connect());
    esp_err_t ret = example_connect();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "example_connect() failed — running in offline mode.");
        wifi_connected = false;
    } else {
        ESP_LOGI(TAG, "example_connect() succeeded");
        wifi_connected = true;
    }

    // Wi-Fi verification task
    xTaskCreate(verify_wifi_connection, "verify_wifi_connection", 4096, NULL, 5, NULL);
}


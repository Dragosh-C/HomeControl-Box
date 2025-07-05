#include <stdio.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "output/led_signals.h"

static const char *TAG = "wifi_setup_box_1000";

#define PULSE_STEP 5
#define PULSE_DELAY 20   

#define SHORT_BLINK_DUTY 245  
#define LITTLE_BLINK_DUTY 256   
#define SHORT_BLINK_TIME pdMS_TO_TICKS(50)
#define LITTLE_BLINK_TIME pdMS_TO_TICKS(500)
#define LITTLE_BLINK_OFF_TIME pdMS_TO_TICKS(5000)


static bool wifi_connected = false;
void pulse_led(void) {
    int duty;
    for (duty = 0; duty <= 255; duty += PULSE_STEP) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL, duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL));
        vTaskDelay(pdMS_TO_TICKS(PULSE_DELAY));
    }

    for (duty = 255; duty >= 0; duty -= PULSE_STEP) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL, duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL));
        vTaskDelay(pdMS_TO_TICKS(PULSE_DELAY));
    }
}

void little_blink_led(void) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL, SHORT_BLINK_DUTY));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL));
    vTaskDelay(SHORT_BLINK_TIME);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL, LITTLE_BLINK_DUTY));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL));
    vTaskDelay(LITTLE_BLINK_OFF_TIME);
}


void connection_output_led(void *param) {
    ESP_LOGI(TAG, "Starting connection output LED task");
    int count_attempts = 0;
    while (!wifi_connected) {

        pulse_led();

        // Check Wi-Fi status
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "Connected to SSID: %s", ap_info.ssid);
            wifi_connected = true;
        }

         count_attempts++;
        if (count_attempts > 15) {
            ESP_LOGE(TAG, "Could not connect to Wi-Fi retrying...");
            ESP_ERROR_CHECK(example_disconnect());
            count_attempts = 0;
            ESP_ERROR_CHECK(example_connect());
        }
    }

    ESP_LOGI(TAG, "Wi-Fi connected, stopping LED task");
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL, 256)); // Turn off LED
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_B_CHANNEL));
    vTaskDelete(NULL); // Delete current task
}

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
                xTaskCreate(connection_output_led, "connection_output_led", 4096, NULL, 5, NULL);
                vTaskDelete(NULL);
            }

            last_check_time = xTaskGetTickCount();
        }

        little_blink_led();
    }
}



void wifi_init_sta() {
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    configure_pwm();

    //LED blinking task
    xTaskCreate(connection_output_led, "connection_output_led", 4096, NULL, 5, NULL);

    // Initialize Wi-Fi
    ESP_ERROR_CHECK(example_connect());

    // Wi-Fi verification task
    xTaskCreate(verify_wifi_connection, "verify_wifi_connection", 4096, NULL, 5, NULL);
}


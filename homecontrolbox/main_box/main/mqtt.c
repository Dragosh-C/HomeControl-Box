#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>
#include "mqtt.h"

#define BROKER_URI "mqtts://192.168.0.101:8883?clientId=esp32_main_box_1000"


static const char *TAG = "main_box_1000_mqtt";

#define ALARM_PIN 19

void init_alarm_pin() {
    gpio_reset_pin(ALARM_PIN); // Reset the pin to avoid conflicts
    gpio_set_direction(ALARM_PIN, GPIO_MODE_OUTPUT);
}

void set_alarm(int state) {
     gpio_set_level(ALARM_PIN, state); 
}

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_ssl_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_ssl_start[]   asm("_binary_ssl_pem_start");
#endif
extern const uint8_t mqtt_ssl_end[]   asm("_binary_ssl_pem_end");


esp_mqtt_client_handle_t client;
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler.
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        
        msg_id = esp_mqtt_client_subscribe(client, "box_id/1111/alarm", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "devices/ports/Port 1/port", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "devices/ports/Port 2/port", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "devices/ports/Port 3/port", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "devices/ports/Port 4/dimmer", 1);
        msg_id = esp_mqtt_client_subscribe(client, "devices/ports/Port 4/port", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/subscribed", "box_id_1111", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if (strncmp (event->topic, "/box_id/1000/alarm", 18) == 0) {
            if (strncmp (event->data, "on", 2) == 0) {
                printf("Alarm is on\n");
                // call the alarm function
                set_alarm(1);


            } else {
                printf("Alarm is off\n");
                // call the alarm function
                set_alarm(0);
            }
        }

        if (strncmp (event->topic, "devices/ports/Port 1/port", 25) == 0) {
            if (strncmp (event->data, "true", 4) == 0) {
                printf("Port 1 is on\n");
                // call the relay function
                set_relays(1, 1);
            } else {
                printf("Port 1 is off\n");
                // call the relay function
                set_relays(1, 0);
            }
        }

        if (strncmp (event->topic, "devices/ports/Port 2/port", 25) == 0) {
            if (strncmp (event->data, "true", 4) == 0) {
                printf("Port 2 is on\n");
                // call the relay function
                set_relays(2, 1);
            } else {
                printf("Port 2 is off\n");
                // call the relay function
                set_relays(2, 0);
            }
        }

        if (strncmp (event->topic, "devices/ports/Port 3/port", 25) == 0) {
            if (strncmp (event->data, "true", 4) == 0) {
                printf("Port 3 is on\n");
                // call the relay function
                set_relays(3, 1);
            } else {
                printf("Port 3 is off\n");
                // call the relay function
                set_relays(3, 0);
            }
        }

        if (strncmp (event->topic, "devices/ports/Port 4/dimmer", 27) == 0) {
            // set dimmer value

            char dimmer_str[4]; 
            memcpy(dimmer_str, event->data, event->data_len);
            dimmer_str[event->data_len] = '\0';  

            int dimmer_value = atoi(dimmer_str);
            printf("Dimmer value: %d\n", dimmer_value);
            set_dimmer(dimmer_value);

            // int dimmer_value = atoi(event->data);
            // printf("Dimmer value: %d\n", dimmer_value);
            // // call the relay function
            // set_dimmer(dimmer_value);
        }

        if (strncmp (event->topic, "devices/ports/Port 4/port", 25) == 0) {
            // set dimmer value

            if (strncmp (event->data, "true", 4) == 0) {
                printf("Port 4 is on\n");
                // call the relay function
                set_dimmer(100);
            } else {
                printf("Port 4 is off\n");
                // call the relay function
                set_dimmer(0);
            }

            // int dimmer_value = atoi(event->data);
            // printf("Dimmer value: %d\n", dimmer_value);
            // // call the relay function
            // set_dimmer(dimmer_value);
        }



        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_publish(const char *topic, const char *message) {
    if ( client != NULL) {
        int msg_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    } else {
        ESP_LOGE(TAG, "MQTT client not initialized");
    }
}



void mqtt_start(void)
{
    init_alarm_pin();
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = BROKER_URI,
            .verification.certificate = (const char *)mqtt_ssl_start
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
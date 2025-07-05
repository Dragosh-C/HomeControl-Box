#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "configure_adc.h"
#include "mqtt.h"
#include <dht.h>
#include <math.h>
#include "driver/gpio.h"
#include "esp_timer.h"

#define SENSOR_TYPE DHT_TYPE_DHT11
#define DHT_GPIO_PIN 19
#define RL 10.0  
#define R0 10.0
#define MAX_SENSOR_VALUE 1023.0
#define ALARM_PIN 14
#define MICROPHONE_PIN  27
#define RELAY_GPIO GPIO_NUM_2
#define DEBOUNCE_TIME 200
#define MAX_CLAP_INTERVAL 800         // Max time between two claps
#define MIN_CLAP_RESET_TIME 1000      // Minimum time before recognizing new claps
#define WATER_LEAK_GPIO 5


static QueueHandle_t gpio_evt_queue = NULL;
static bool relay_status = false;
static long lastNoiseTime = 0;
static long lastLightChange = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int gpio_num = (int)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void clap_task(void *arg)
{
    int io_num;
    while (true)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            long currentNoiseTime = esp_timer_get_time() / 1000; // Convert to milliseconds
            
            if (
                (currentNoiseTime > lastNoiseTime + DEBOUNCE_TIME) &&
                (currentNoiseTime < lastNoiseTime + MAX_CLAP_INTERVAL) &&
                (currentNoiseTime > lastLightChange + MIN_CLAP_RESET_TIME))
            {
                relay_status = !relay_status;
                // gpio_set_level(RELAY_GPIO, relay_status);
                ESP_LOGI("CLAPSSSSS", "Claps detected:");

                // const char *topic = "ports/Port1";
                // if (relay_status)
                //     mqtt_publish("/ports/Port 1", "true");
                // else
                //     mqtt_publish("/ports/Port 1", "false");

                char path[23];

                sprintf(path, "dev/box_id/%s/clap", BOX_ID);

                if (relay_status)
                    mqtt_publish(path, "true");
                else
                    mqtt_publish(path, "false");
              
                lastLightChange = currentNoiseTime;
            }
            lastNoiseTime = currentNoiseTime;
        }
    }
}



void init_mic_pin() {
    gpio_reset_pin(MICROPHONE_PIN); // Reset the pin to avoid conflicts
    gpio_set_direction(MICROPHONE_PIN, GPIO_MODE_INPUT);
}


float approximateCO2(int sensorValue) {

    return 4500.0 * (sensorValue / MAX_SENSOR_VALUE); 
}

// Map CO2 ppm to AQI
float mapToAQI(float concentration) {
    if (concentration <= 400)
        return (concentration * 50) / 400.0; // (0-50)
    else if (concentration <= 1000)
        return (((concentration - 401) * 49) / 599.0) + 51; //(51-100)
    else if (concentration <= 2000)
        return (((concentration - 1001) * 49) / 999.0) + 101; // (101-150)
    else
        return 301; //(301+)
}
const static char *TAG = "MQ2_SENSOR";



void dht_task(void *arg)
{
    int16_t temperature, humidity;
    char str_val[10];
    gpio_reset_pin(DHT_GPIO_PIN);
    gpio_set_direction(DHT_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT_OD);  // Open-drain
    gpio_set_pull_mode(DHT_GPIO_PIN, GPIO_PULLUP_ONLY);

    while (1)
    {

        int res = dht_read_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature);

        if (res == ESP_OK)
        {
            sprintf(str_val, "%d", temperature / 10);
            mqtt_publish("dev/box_id/1212/temperature", str_val);
            sprintf(str_val, "%d", humidity / 10);
            mqtt_publish("dev/box_id/1212/humidity", str_val);
            ESP_LOGI("DHT", "Temperature: %d, Humidity: %d", temperature / 10, humidity / 10);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}




// Convert ADC value to voltage
float adc_to_voltage(int adc_value, float v_ref) {
    return (adc_value / MAX_SENSOR_VALUE) * v_ref;
}

#define NUM_SAMPLES 10  // Number of samples to average

float v_ref = 3.64;       // Reference voltage
float read_stable_voltage() {
    float total = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int raw_value = read_adc1_channel(ADC_CHANNEL_BAT);
        total += adc_to_voltage(raw_value, v_ref);
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
    float average_voltage = total / NUM_SAMPLES;
    return average_voltage * 2; 
}

int prev_time_value = 0;

void read_sensors(void *pvParameters) {

    // Microphone

    init_mic_pin();

    // create a task for mic pin
    gpio_config_t io_conf1 = {
        .intr_type = GPIO_INTR_POSEDGE, // Interrupt on rising edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << MICROPHONE_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf1);


     gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WATER_LEAK_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);



    // gpio_config_t relay_conf = {
    //     .mode = GPIO_MODE_OUTPUT,
    //     .pin_bit_mask = (1ULL << RELAY_GPIO),
    //     .pull_up_en = GPIO_PULLUP_DISABLE,
    //     .pull_down_en = GPIO_PULLDOWN_DISABLE
    // };
    // gpio_config(&relay_conf);
    // gpio_set_level(RELAY_GPIO, relay_status);

    gpio_evt_queue = xQueueCreate(10, sizeof(int));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(MICROPHONE_PIN, gpio_isr_handler, (void *)MICROPHONE_PIN);

    xTaskCreate(clap_task, "clap_task", 4048, NULL, 10, NULL);
     // create a task for dht11
    xTaskCreate(dht_task, "dht_task", 4048, NULL, 10, NULL);

    int16_t raw_value = 0;
    int16_t prev_light_intensity = 0;
    
    float AQI_float, CO2_ppm_float;
    int16_t AQI;
    int8_t threshold = 1;
    int16_t prev_AQI = 0;
    float voltage;      

    char str_val[10];
    
    while (1) {
        // MQ2 sensor
        raw_value = read_adc1_channel(ADC_CHANNEL_MQ);
        ESP_LOGI(TAG, "Raw ADC Data from MQ2 Sensor: %d", raw_value);    
        CO2_ppm_float = approximateCO2(raw_value);
        AQI_float = mapToAQI(CO2_ppm_float);
        AQI = (int16_t)(AQI_float + 0.5);      
        ESP_LOGI(TAG, "AQI: %d", AQI);
       
        
        if (abs(AQI - prev_AQI) > threshold) {
            sprintf(str_val, "%d", AQI);
            mqtt_publish("dev/box_id/1212/air_quality", str_val);
            prev_AQI = AQI;
            if (AQI > 300) {
                // turn on the alarm
                gpio_set_level(ALARM_PIN, 1);
            }
        }

        // Photoresistor
        raw_value = read_adc1_channel(ADC_CHANNEL_LDR);
        ESP_LOGI(TAG, "Raw ADC Data from Photoresistor: %d", raw_value);
        sprintf(str_val, "%d", raw_value);

        if (abs(raw_value - prev_light_intensity) > 50) {
            sprintf(str_val, "%d", raw_value);
            mqtt_publish("dev/box_id/1212/light_intensity", str_val);
            prev_light_intensity = raw_value;
        }

        // Battery voltage
        raw_value = read_adc1_channel(ADC_CHANNEL_BAT);

        voltage = adc_to_voltage(raw_value, v_ref);

        ESP_LOGI(TAG, "Raw ADC Data from Battery: %f", voltage * 2 );
        sprintf(str_val, "%f", read_stable_voltage());

        // if 5 min passed, publish battery voltage

        if ((xTaskGetTickCount() - prev_time_value) > 30000) {
            mqtt_publish("dev/box_id/1212/battery_voltage", str_val);
            prev_time_value = xTaskGetTickCount();
        }


        // DHT11 sensor
        // if (dht_read_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature) == ESP_OK) {

        //     if (abs(temperature - prev_temperature) > threshold) {
        //         sprintf(str_val, "%d", temperature / 10);
        //         mqtt_publish("dev/box_id/1212/temperature", str_val);
        //         prev_temperature = temperature;

        //     }

        //     if (abs(humidity - prev_humidity) > threshold) {
        //         sprintf(str_val, "%d", humidity / 10);
        //         mqtt_publish("dev/box_id/1212/humidity", str_val);
        //         prev_humidity = humidity;
        //     }

        // } else {
        //     ESP_LOGE(TAG, "Could not read data from DHT11 sensor !");
        //     int count = 3;
        //     while (count--) {
                
        //         if (dht_read_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature) == ESP_OK) {
                    
        //             if (abs(temperature - prev_temperature) > threshold) {
        //                 sprintf(str_val, "%d", temperature / 10);
        //                 mqtt_publish("dev/box_id/1212/temperature", str_val);
        //                 prev_temperature = temperature;
        
        //             }
        
        //             if (abs(humidity - prev_humidity) > threshold) {
        //                 sprintf(str_val, "%d", humidity / 10);
        //                 mqtt_publish("dev/box_id/1212/humidity", str_val);
        //                 prev_humidity = humidity;
        //             }
        //             break;
        //         }
        //     }
        // }   



        int level = gpio_get_level(WATER_LEAK_GPIO);
        if (level == 1) {
            ESP_LOGI(TAG, "Water leak detected!");
            mqtt_publish("dev/box_id/1212/water_leak", "1");
        } else {
            ESP_LOGI(TAG, "No water leak detected.");
            mqtt_publish("dev/box_id/1212/water_leak", "0");
        }




        vTaskDelay(pdMS_TO_TICKS(7000));
    }

}
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "read_sensors.h"
#include "configure_adc.h"
#include "mqtt.h"
#include "output/led_signals.h"
#include "IR/ir_sensor.h"

#define BUZZER_PIN 14
#define MQ2_PIN 39
#define DHT11_PIN 13
#define PHOTORESISTOR_PIN 34
#define MICROPHONE_PIN 27 
#define IR_RECEIVE_PIN 36
#define LIGHT_PWM_PIN 16
#define ALIM_CT_3_3V 19
#define CTSENSOR_PIN 4

static const char *TAG = "sensor";

void init_sensors() {
    init_adc1();
}
#define PIN 13

void sensor_func() {

    init_sensors();

    xTaskCreate(read_sensors, "read_sensors", 8048, NULL, 5, NULL);

    read_ir();
}

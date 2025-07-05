
#include "driver/gpio.h"
#include "mqtt.h"
#include "led_signals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RELLAY_1 12
#define RELLAY_2 14
#define RELLAY_3 27
#define RELLAY_4 26

void init_relays(){

    // gpio_pad_select_gpio(RELLAY_1);
    gpio_set_direction(RELLAY_1, GPIO_MODE_OUTPUT);
    // gpio_set_level(RELLAY_1, 0);

    // gpio_pad_select_gpio(RELLAY_2);
    gpio_set_direction(RELLAY_2, GPIO_MODE_OUTPUT);
    // gpio_set_level(RELLAY_2, 0);

    // gpio_pad_select_gpio(RELLAY_3);
    gpio_set_direction(RELLAY_3, GPIO_MODE_OUTPUT);
    // gpio_set_level(RELLAY_3, 0);

    // gpio_pad_select_gpio(RELLAY_4);
    gpio_set_direction(RELLAY_4, GPIO_MODE_OUTPUT);
    // gpio_set_level(RELLAY_4, 0);
}


void set_dimmer(int duty){
    // set the pwm value to the pin

    duty = 255 - (duty * 255) / 100;

    ledc_set_duty(LEDC_MODE, LEDC_OUTPUT_DIMMER_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_OUTPUT_DIMMER_CHANNEL);

}


void set_relays(int8_t relay, int8_t state) {
    switch (relay) {
        case 4:
            gpio_set_level(RELLAY_1, state);
            break;
        case 2:
            gpio_set_level(RELLAY_2, state);
            break;
        case 3:
            gpio_set_level(RELLAY_3, state);
            break;
        case 1:
            gpio_set_level(RELLAY_4, state);
            break;
        default:
            break;
    }

}

void relays_func(){

    // Create a task to control the relays

    init_relays();

}
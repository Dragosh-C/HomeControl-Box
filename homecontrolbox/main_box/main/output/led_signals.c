#include "led_signals.h"

void configure_pwm(){

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configuration for blue LED
    ledc_channel_config_t ledc_channel_b = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_OUTPUT_B_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN_B,
        .duty           = 256, // Led off
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_b));

    // Configuration dimmer

    ledc_channel_config_t ledc_channel_g = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_OUTPUT_DIMMER_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN_DIMMER,
        .duty           = 220, // Led off
        .hpoint         = 0
    };



    // Coniguration for 

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_g));
} 

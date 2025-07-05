#include "driver/ledc.h"

#define LED_PIN_B 32
#define LED_PIN_DIMMER 23
#define LED_PIN_R 26

#define LEDC_OUTPUT_R_CHANNEL   LEDC_CHANNEL_0
#define LEDC_OUTPUT_DIMMER_CHANNEL   LEDC_CHANNEL_1
#define LEDC_OUTPUT_B_CHANNEL   LEDC_CHANNEL_2
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Duty resolution  8 bits
#define LEDC_DUTY               (0)
#define LEDC_FREQUENCY          (4000) // Frequency 4 kHz

void configure_pwm();

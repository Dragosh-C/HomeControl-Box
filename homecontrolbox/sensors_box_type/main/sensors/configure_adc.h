#include <stdint.h>
#include "esp_adc/adc_oneshot.h"
#define ADC_CHANNEL_MQ   ADC_CHANNEL_3  // MQ sensor connected to ADC1 Channel 3 (pin 39)
#define ADC_CHANNEL_LDR  ADC_CHANNEL_6  // Photoresistor connected to ADC1 Channel 6 (pin 34)
#define ADC_CHANNEL_BAT  ADC_CHANNEL_5 // Battery voltage connected to ADC1 Channel 5 (pin 33)

void init_adc1();
int16_t read_adc1_channel(adc_channel_t channel);
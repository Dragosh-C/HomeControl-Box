#include <stdint.h>
#include "esp_adc/adc_oneshot.h"

#define ADC_CHANNEL_CTSENSOR ADC_CHANNEL_5 // CTSensor connected to ADC1 Channel 5 (pin 33)
#define ADC_CHANNEL_MQ   ADC_CHANNEL_6  // MQ sensor connected to ADC1 (pin 34)
#define ADC_CHANNEL_LDR  ADC_CHANNEL_7  // Photoresistor connected to ADC1 (pin 35)
// #define ADC_CHANNEL_IR   ADC_CHANNEL_3 // IR sensor connected to ADC1 (pin 39)

void init_adc1();
int16_t read_adc1_channel(adc_channel_t channel);
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "configure_adc.h"

#define ADC_UNIT ADC_UNIT_1
#define ADC_BITWIDTH ADC_BITWIDTH_10
#define ADC_ATTEN ADC_ATTEN_DB_12   // Attenuation for 0.0V - 3.6V range

static const char *TAG = "ADC";
static adc_oneshot_unit_handle_t adc_handle;


void init_adc1(){
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return;
    }

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };

    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_MQ, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel %d: %s", ADC_CHANNEL_MQ, esp_err_to_name(ret));
    }

    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_LDR, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel %d: %s", ADC_CHANNEL_LDR, esp_err_to_name(ret));
    }

    // ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_IR, &chan_config);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to configure ADC channel %d: %s", ADC_CHANNEL_IR, esp_err_to_name(ret));
    // }

    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_CTSENSOR, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel %d: %s", ADC_CHANNEL_CTSENSOR, esp_err_to_name(ret));
    }
}

int16_t read_adc1_channel(adc_channel_t channel) {
    int raw_value = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, channel, &raw_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC value from channel %d: %s", channel, esp_err_to_name(ret));
        return -1;
    }

    ESP_LOGI(TAG, "Raw ADC Value from channel %d: %d", channel, raw_value);
    return raw_value;
}

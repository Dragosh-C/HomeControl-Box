#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"


void interpret_command(uint16_t command, int8_t repeat);
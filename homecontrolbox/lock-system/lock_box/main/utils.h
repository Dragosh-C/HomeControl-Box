#ifndef UTILS_H
#define UTILS_H

#include "lcd1602.h"
#include "mbedtls/sha256.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#define NUM_ROWS 4
#define NUM_COLS 3


esp_err_t save_password_hash_to_flash(const uint8_t hash[32]);
esp_err_t load_password_hash_from_flash(uint8_t hash_out[32]);


extern const gpio_num_t row_pins[NUM_ROWS];
extern const gpio_num_t col_pins[NUM_COLS];
extern const char keymap[NUM_ROWS][NUM_COLS];


void hash_password_sha256(const char *input, uint8_t output_hash[32]);
void print_hash(uint8_t hash[32]);

void init_keypad();
char scan_keypad();


#endif /* UTILS_H */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd1602.h"
#include "mbedtls/sha256.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "utils.h"
#include "rfid_read.h"
#include "mqtt.h"
#include "wifi_setup.h"
#include "esp_netif.h"
#include <esp_timer.h>
#include "driver/adc.h"
#include "driver/ledc.h"
char password[7] = {0}; 

void lock_doors(int8_t *doors_locked)
{
    // count ten seconds then lock the doors
    for (int i = 10; i > 0; i--)
    {
        lcd_clear();
        lcd_put_cursor(0, 0);
        lcd_send_string("Locking in: ");
        // print the countdown on the LCD
        lcd_put_cursor(1, 0);
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d seconds", i);
        lcd_send_string(buffer);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    *doors_locked = true;
    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Enter Password:");
}

void lock_mode(char *password, int count, uint8_t stored_hash[32], int8_t *doors_locked)
{
    // print Lock password text
    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Lock password");
    lcd_put_cursor(1, 0);

    password[count - 1] = '\0'; // Remove the '#' character
    count = 0;

    // enter the password
    char lock_key[7] = {0};
    int lock_count = 0;

    // after 10 seconds exit from # loop if no key is pressed
    int timeout = 10000; // 10 seconds
    int elapsed_time = 0;
    char key = '\0';
    while (lock_count < 6)
    {
        key = scan_keypad();
        if (key != '\0')
        {
            lock_key[lock_count++] = key;
            lock_key[lock_count] = '\0';
            lcd_send_string("*");
            printf("Key Pressed: %c\n", key);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed_time += 100; 
        if (elapsed_time >= timeout)
        {
            lcd_clear();
            lcd_put_cursor(0, 0);
            lcd_send_string("Enter password:");
            lcd_put_cursor(1, 0);
            return; // Exit the loop after 10 seconds
        }
    }

    // Check if the entered password matches the stored password
    uint8_t lock_hash[32];
    hash_password_sha256(lock_key, lock_hash);
    if (memcmp(lock_hash, stored_hash, 32) == 0)
    {
        // printf("Lock password correct\n");
        lock_doors(doors_locked);
    }
    else
    {
        // printf("Lock password incorrect\n");

    }

    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Enter password:");
    lcd_put_cursor(1, 0);
}

void change_password() {

    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Enter new pin:");
    lcd_put_cursor(1, 0);

    
    // enter the password
    char new_password[7] = {0};
    int count = 0;
    int timeout = 10000000; // 10 seconds
    int elapsed_time = 0;
    char key = '\0';
    while (count < 6)
    {
        key = scan_keypad();
        if (key != '\0')
        {
            new_password[count++] = key;
            new_password[count] = '\0';
            char str_key[2] = {key, '\0'};
            lcd_send_string(str_key);
            // printf("Key Pressed: %c\n", key);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed_time += 100; // Increment elapsed time by 100 ms
        // if (elapsed_time >= timeout)
        // {
        //     // printf("Timeout reached, exiting password entry.\n");
        //     lcd_clear();
        //     lcd_put_cursor(0, 0);
        //     lcd_send_string("Menu Mode");
        //     lcd_put_cursor(1, 0);
        //     return; // Exit the loop after 10 seconds
        // }
    }

    // hash new password and store in flash;

    printf("PASSSSSSSSSS%s\n", new_password);

    uint8_t hash[32];
    hash_password_sha256(new_password, hash);
    save_password_hash_to_flash(hash);
    abort();

}


void enter_menu_mode()
{
    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Menu Mode");
    lcd_put_cursor(1, 0);


    static const char* menu_msg = "Press * to exit   Press 1 to change password\
       Press 2 to add a new RFID    Press 3 to delete RFID card";

    char display_band[17] = {0}; // Holds 16 chars + null terminator
int8_t msg_length = strlen(menu_msg);

long current_time = esp_timer_get_time();
int8_t i = 0;
while (1)
{
    char key = scan_keypad();

    if (esp_timer_get_time() - current_time > 300000) {
    current_time = esp_timer_get_time();

    lcd_put_cursor(1, 0); 

    memset(display_band, ' ', 16);

    for (int8_t j = 0; j < 16; j++) {
        if (i + j < msg_length)
            display_band[j] = menu_msg[i + j];
        else
            display_band[j] = ' ';
    }
    display_band[16] = '\0';

    lcd_send_string(display_band);

    i++;
    if (i >= msg_length - 15) i = 0;
}


    if (key == '*') {
        lcd_clear();
        lcd_put_cursor(0, 0);
        lcd_send_string("Enter password:");
        lcd_put_cursor(1, 0);
        break;
    }

    if (key == '1') {
        change_password();
    }

    if (key == '2') {
        add_rfid = true;
        lcd_clear();
        lcd_put_cursor(0, 0);
        lcd_send_string("Bring the card");

    }

    if (key == '3') {
        delete_rfid = true;
        lcd_clear();
        lcd_put_cursor(0, 0);
        lcd_send_string("Bring the card");
    }

    vTaskDelay(pdMS_TO_TICKS(200));
}

}

void configure_servo() {

       ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,      // 10-bit resolution (0-1023)
        .freq_hz          = 50,                     // 50 Hz for servo
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = 10,   // GPIO10 for servo
        .duty           = 0,    // Initial duty
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);


}

void app_main(void)
{
    lcd_init();
    lcd_clear();
    init_keypad();


    adc1_config_width(ADC_WIDTH_BIT_12); 
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11); // For 0-3.3V
    
    configure_servo();



    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    rfid_init();
    wifi_init_sta();
    mqtt_start();

    // Load the password hash from flash
    uint8_t stored_hash[32];
    esp_err_t err = load_password_hash_from_flash(stored_hash);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // If no hash is found, create a default password and save it
        const char *default_password = "123456";
        uint8_t default_hash[32];
        hash_password_sha256(default_password, default_hash);
        save_password_hash_to_flash(default_hash);
    }

    char password[7] = {0}; // Store up to 6 digits + null terminator
    int count = 0;
    // int8_t doors_locked = false;

    lcd_put_cursor(0, 0);
    lcd_send_string("Enter password:");
    lcd_put_cursor(1, 0);
    int count_stars = 0;
    char stars[7] = {0};
    int8_t stars_count = 0;
    int8_t last_hall_state = 0;

    int8_t prev_door_locked_state = 0;
    while (1)
    {   
    // Read the gpio 1 pin adc pin value and print in console
        int adc_value = adc1_get_raw(ADC1_CHANNEL_1);
        if (adc_value > 1000) adc_value = 1;
        else adc_value = 0;
        if (adc_value != last_hall_state) {
            mqtt_publish("dev/box_id/4123/hall_state", adc_value ? "1" : "0");
            last_hall_state = adc_value;
        }

        // printf("GPIO 1 ADC Value: %d\n", adc_value);
        // if (adc_value > 500) {
        //     printf("GPIO 1 is LOW\n");
        //         ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 33); // 75% duty
        //         ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        // } else
        // {
        //     printf("GPIO 1 is HIGH\n");
        //         ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1000); // 75% duty
        //         ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        // }

        if (doors_locked) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1000); 
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }
        else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 33); 
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }


        if (doors_locked != prev_door_locked_state) {
            if (doors_locked) {
                mqtt_publish("dev/box_id/4123/door_lock", "1");
            } else {
                mqtt_publish("dev/box_id/4123/door_lock", "0");
            }
            prev_door_locked_state = doors_locked;
        }


        if (!enable_pin) {
            lcd_put_cursor(0, 0);
            lcd_send_string("Access with pin");
            lcd_put_cursor(1, 0);
            lcd_send_string("disabled");
            continue;
        }

        char key = scan_keypad();

        if (doors_locked)
        {
            lcd_clear();
            lcd_put_cursor(0, 0);
            lcd_send_string("Enter password:");
            lcd_put_cursor(1, 0);
            lcd_send_string(stars);
        }
        else
        {
            lcd_clear();
            lcd_put_cursor(0, 0);
            lcd_send_string("Welcome!");
            lcd_put_cursor(1, 0);
            lcd_send_string(stars);
        }

        if (key != '\0' && count < 6)
        {
            password[count++] = key;
            password[count] = '\0';

            // lcd_send_string("*"); // or show the actual char
            stars[stars_count++] = '*';
            stars[stars_count] = '\0'; // Null-terminate the string
            lcd_put_cursor(1, 0);
            lcd_send_string(stars); // Display the stars on the second row

            // printf("Key Pressed: %c\n", key);

            if (key == '#')
            {
                lock_mode(password, count, stored_hash, &doors_locked);
                count = 0;                             // Reset count after locking
                memset(password, 0, sizeof(password)); // Clear password buffer
                stars_count = 0;                       // Reset stars count
                stars[0] = '\0';                       // Clear stars
            }

            if (key == '*')
            {
                // count 5 rapid presse
                count_stars++;

                if (count_stars == 5 && doors_locked == false)
                {
                    enter_menu_mode();
                    count_stars = 0;
                    count = 0;
                    memset(password, 0, sizeof(password));
                    stars_count = 0;
                    stars[0] = '\0';
                    lcd_clear();
                }
            }
            else
            {
                count_stars = 0; // Reset if a different key is pressed
            }
        }

        if (count == 6)
        {
            // Hash the password
            uint8_t hash[32];
            hash_password_sha256(password, hash);
            uint8_t hash_cmp[32];
            int err = load_password_hash_from_flash(hash_cmp);
            if (err != ESP_OK)
            {
                // printf("Error loading hash from flash: %s\n", esp_err_to_name(err));
            }
            else
            {
                if (memcmp(hash, hash_cmp, 32) == 0)
                {
                    lcd_clear();
                    lcd_put_cursor(0, 0);
                    lcd_send_string("Welcome!");
                    lcd_put_cursor(1, 0);
                    // printf("Access Granted!\n");
                    doors_locked = false; // Unlock doors
                }
                else
                {
                    lcd_clear();
                    lcd_put_cursor(0, 0);
                    lcd_send_string("Access Denied!");
                    // printf("Access Denied!\n");
                }
            }

            print_hash(hash);

            // Reset state
            count = 0;
            memset(password, 0, sizeof(password));

            vTaskDelay(pdMS_TO_TICKS(1000));
            lcd_clear();
            stars_count = 0;
            stars[0] = '\0'; // Clear stars
            lcd_put_cursor(0, 0);
            lcd_send_string("Enter password:");
            lcd_put_cursor(1, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
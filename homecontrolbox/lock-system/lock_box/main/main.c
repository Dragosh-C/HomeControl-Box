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

void lock_doors(int8_t *doors_locked)
{
    // Simulate locking doors
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
        elapsed_time += 100; // Increment elapsed time by 100 ms
        if (elapsed_time >= timeout)
        {
            printf("Timeout reached, exiting lock password entry.\n");
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
        printf("Lock password correct\n");
        lock_doors(doors_locked);
    }
    else
    {
        printf("Lock password incorrect\n");
    }

    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Enter password:");
    lcd_put_cursor(1, 0);
}

void enter_menu_mode()
{
    lcd_clear();
    lcd_put_cursor(0, 0);
    lcd_send_string("Menu Mode");
    lcd_put_cursor(1, 0);

    lcd_send_string("Press * to exit");

    while (1)
    {
        char key = scan_keypad();
        if (key == '*')
        {
            lcd_clear();
            lcd_put_cursor(0, 0);
            lcd_send_string("Enter password:");
            lcd_put_cursor(1, 0);
            break; // Exit menu mode
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    lcd_init();
    lcd_clear();
    init_keypad();

    
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
    while (1)
    {
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

            printf("Key Pressed: %c\n", key);

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

                if (count_stars == 5)
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
                printf("Error loading hash from flash: %s\n", esp_err_to_name(err));
            }
            else
            {
                if (memcmp(hash, hash_cmp, 32) == 0)
                {
                    lcd_clear();
                    lcd_put_cursor(0, 0);
                    lcd_send_string("Welcome!");
                    lcd_put_cursor(1, 0);
                    printf("Access Granted!\n");
                    doors_locked = false; // Unlock doors
                }
                else
                {
                    lcd_clear();
                    lcd_put_cursor(0, 0);
                    lcd_send_string("Access Denied!");
                    printf("Access Denied!\n");
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

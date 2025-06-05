#include "utils.h"


const gpio_num_t row_pins[NUM_ROWS] = {GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5};
const gpio_num_t col_pins[NUM_COLS] = {GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8};

const char keymap[NUM_ROWS][NUM_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

esp_err_t save_password_hash_to_flash(const uint8_t hash[32])
{
    nvs_handle_t handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_blob(handle, "pwd_hash", hash, 32);
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
}

esp_err_t load_password_hash_from_flash(uint8_t hash_out[32])
{
    nvs_handle_t handle;
    esp_err_t err;
    size_t required_size = 32;

    err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_get_blob(handle, "pwd_hash", hash_out, &required_size);

    nvs_close(handle);
    return err;
}


void hash_password_sha256(const char *input, uint8_t output_hash[32])
{
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // Use SHA-256
    mbedtls_sha256_update(&ctx, (const unsigned char *)input, strlen(input));
    mbedtls_sha256_finish(&ctx, output_hash);
    mbedtls_sha256_free(&ctx);
}

void print_hash(uint8_t hash[32])
{
    printf("SHA-256 Hash: ");
    for (int i = 0; i < 32; i++)
    {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

void init_keypad()
{
    for (int i = 0; i < NUM_ROWS; i++)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << row_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);
    }

    for (int i = 0; i < NUM_COLS; i++)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << col_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);
        gpio_set_level(col_pins[i], 1); // Set to HIGH
    }
}

char scan_keypad()
{
    for (int col = 0; col < NUM_COLS; col++)
    {
        gpio_set_level(col_pins[col], 0);

        for (int row = 0; row < NUM_ROWS; row++)
        {
            if (gpio_get_level(row_pins[row]) == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(20));
                if (gpio_get_level(row_pins[row]) == 0)
                {
                    while (gpio_get_level(row_pins[row]) == 0)
                    {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }

                    gpio_set_level(col_pins[col], 1);
                    return keymap[row][col];
                }
            }
        }

        gpio_set_level(col_pins[col], 1);
    }

    return '\0';
}

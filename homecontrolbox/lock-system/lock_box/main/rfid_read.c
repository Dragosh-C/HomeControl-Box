#include "rfid_read.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lcd1602.h"
#include "utils.h"


static rc522_driver_handle_t driver;
static rc522_handle_t scanner;


#define MAX_CARDS 50
#define UID_STR_LEN 32

int8_t doors_locked = false;
int8_t add_rfid = false;
int8_t delete_rfid = false;

static void make_key_name(int index, char *key_out, size_t key_size)
{
    snprintf(key_out, key_size, "uuid_%d", index);
}

bool is_uuid_authorized(const char *uid)
{
    nvs_handle_t handle;
    char key[16], stored_uid[UID_STR_LEN + 1];
    esp_err_t err = nvs_open("rfid", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    for (int i = 0; i < MAX_CARDS; i++) {
        size_t len = sizeof(stored_uid);
        make_key_name(i, key, sizeof(key));
        err = nvs_get_str(handle, key, stored_uid, &len);

        if (err == ESP_OK && strcmp(uid, stored_uid) == 0) {
            nvs_close(handle);
            return true;
        }
    }

    nvs_close(handle);
    return false;
}

esp_err_t add_uuid_to_flash(const char *uid)
{
    if (is_uuid_authorized(uid)) {
        ESP_LOGI(TAG, "UID already exists.");
        return ESP_OK;
    }

    nvs_handle_t handle;
    char key[16];
    char stored_uid[UID_STR_LEN + 1];
    esp_err_t err = nvs_open("rfid", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    for (int i = 0; i < MAX_CARDS; i++) {
        size_t len = sizeof(stored_uid);
        make_key_name(i, key, sizeof(key));
        err = nvs_get_str(handle, key, stored_uid, &len);

        if (err == ESP_ERR_NVS_NOT_FOUND) {
            err = nvs_set_str(handle, key, uid);
            if (err == ESP_OK) {
                nvs_commit(handle);
                ESP_LOGI(TAG, "UID added as %s", key);
            }
            nvs_close(handle);
            return err;
        }
    }

    nvs_close(handle);
    ESP_LOGW(TAG, "UID list full");
    return ESP_ERR_NO_MEM;
}

esp_err_t delete_uuid_from_flash(const char *uid)
{
    nvs_handle_t handle;
    char key[16], stored_uid[UID_STR_LEN + 1];
    esp_err_t err = nvs_open("rfid", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    for (int i = 0; i < MAX_CARDS; i++) {
        size_t len = sizeof(stored_uid);
        make_key_name(i, key, sizeof(key));
        err = nvs_get_str(handle, key, stored_uid, &len);

        if (err == ESP_OK && strcmp(uid, stored_uid) == 0) {
            err = nvs_erase_key(handle, key);
            if (err == ESP_OK) {
                nvs_commit(handle);
                // ESP_LOGI(TAG, "UID %s deleted from %s", uid, key);
            }
            nvs_close(handle);
            return err;
        }
    }

    nvs_close(handle);
    // ESP_LOGW(TAG, "UID not found");
    return ESP_ERR_NOT_FOUND;
}

void list_all_uuids()
{
    nvs_handle_t handle;
    char key[16], stored_uid[UID_STR_LEN + 1];
    esp_err_t err = nvs_open("rfid", NVS_READONLY, &handle);
    if (err != ESP_OK) return;

    // ESP_LOGI(TAG, "Authorized UIDs:");
    for (int i = 0; i < MAX_CARDS; i++) {
        size_t len = sizeof(stored_uid);
        make_key_name(i, key, sizeof(key));
        err = nvs_get_str(handle, key, stored_uid, &len);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Slot %d: %s", i, stored_uid);
        }
    }

    nvs_close(handle);
}


static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;


    char uid_str[UID_STR_LEN + 1] = {0};
    for (int i = 0; i < picc->uid.length; i++) {
    sprintf(uid_str + i * 2, "%02X", picc->uid.value[i]);
    }
    // uid_str[picc->uid.length * 2] = '\0'; 


    printf("UIDDDDD: %s", uid_str);

    if (add_rfid) {
        add_uuid_to_flash(uid_str);
        add_rfid = false;
        lcd_clear();
        lcd_put_cursor(0, 0);
        lcd_send_string("RFID added");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (delete_rfid) {
        delete_uuid_from_flash(uid_str);
        delete_rfid = false;
        lcd_clear();
        lcd_put_cursor(0, 0);
        lcd_send_string("RFID deleted");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

     uid_str[picc->uid.length * 2] = '\0'; 
    ESP_LOGI(TAG, "Scanned UID: %s", uid_str);

    if (is_uuid_authorized(uid_str) && enable_rfid) {
        // open_door(); // implement your relay control
        ESP_LOGI(TAG, "Authorized card detected: %s", uid_str);
        doors_locked = false; 
    } else {
        ESP_LOGW(TAG, "Unauthorized card");
    }


    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        rc522_picc_print(picc);
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "Card has been removed");
    }
}


void rfid_init() {
// {   add_uuid_to_flash("23E8ECA7"); // for testing
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
    rc522_start(scanner);
}

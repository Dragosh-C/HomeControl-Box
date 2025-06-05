#ifndef RFID_H
#define RFID_H

#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

extern int8_t doors_locked; // Global variable to track door lock state

static const char *TAG = "lock_4123_mqtt";

#define RC522_SPI_BUS_GPIO_MISO    (23)
#define RC522_SPI_BUS_GPIO_MOSI    (22)
#define RC522_SPI_BUS_GPIO_SCLK    (15)
#define RC522_SPI_SCANNER_GPIO_SDA (18)
#define RC522_SCANNER_GPIO_RST     (-1) // soft-reset

static rc522_spi_config_t driver_config = {
    .host_id = SPI2_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};

static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data);

void rfid_init();

#endif // RFID_H
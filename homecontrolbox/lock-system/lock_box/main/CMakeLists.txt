idf_component_register(SRCS "wifi_setup.c" 
                            "mqtt.c" 
                            "main.c" 
                            "lcd1602.c" 
                            "utils.c" 
                            "rfid_read.c"

                    INCLUDE_DIRS ".")
                    # REQUIRES i2cdev mbedtls nvs_flash mqtt esp_wifi protocol_examples_common)

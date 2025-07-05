#include "interpret_cmd.h"
#include "mqtt.h"


#define ALARM_PIN 19

int8_t alarm_state = 0;
int16_t count_repeat = 0;
int8_t prev_state_port1 = 0;
int8_t prev_state_port2 = 0;
int8_t prev_state_port3 = 0;

int8_t light_intensity = 0;



void short_beep() {
    gpio_set_level(ALARM_PIN, 1);
    vTaskDelay(30 / portTICK_PERIOD_MS);
    gpio_set_level(ALARM_PIN, 0);
}

void interpret_command(uint16_t command, int8_t repeat) {
    char light_intensity_str[8];
    switch (command) {
        case 0xBF40:
            printf("OK\n");
            
            // if (repeat == 1) {
            //     break;
            // }
            // if (alarm_state == 0) {
            //     gpio_set_level(ALARM_PIN, 1); 
            //     alarm_state = 1;
            // } else {
            //     gpio_set_level(ALARM_PIN, 0); 
            //     alarm_state = 0;
            // }
            short_beep();

            break;
        case 0xB946:
            printf("up\n");

            if (repeat == 0) count_repeat = 0;
            count_repeat++;
            if (count_repeat >= 10) {
                // short_beep();

                
                
                if (alarm_state == 0) {
                    gpio_set_level(ALARM_PIN, 1); 
                    alarm_state = 1;
                    mqtt_publish("dev/box_id/1111/alarm", "true");
                } else {
                    gpio_set_level(ALARM_PIN, 0); 
                    alarm_state = 0;
                    mqtt_publish("dev/box_id/1111/alarm", "false");
                }
                
                // if (repeat == 1) {
                //     break;
                // }

                count_repeat = 0;
                // funct to do somehting
            }

            break;
        case 0xEA15:

            printf("down\n");
            if (repeat == 1) break;
            short_beep();

            break;
        case 0xBB44:
            printf("left\n");
            light_intensity -= 5;
            if (light_intensity < 0) light_intensity = 0;
            // convert to string
            set_dimmer(light_intensity);
            // sprintf(light_intensity_str, "%d", light_intensity);
            // mqtt_publish("dev/devices/ports/Port 4/dimmer", light_intensity_str);
            break;
        case 0xBC43:
            printf("right\n");
            light_intensity += 5;
            if (light_intensity > 100) light_intensity = 100;
            set_dimmer(light_intensity);
            // sprintf(light_intensity_str, "%d", light_intensity);
            // mqtt_publish("dev/devices/ports/Port 4/dimmer", light_intensity_str);
            
            break;

        
        // one

        case 0xE916:
            printf("1\n");


            if (repeat == 1) break;
            short_beep();
            if (prev_state_port1 == 0) {
                prev_state_port1 = 1;
                mqtt_publish("dev/ports/Port 1", "false");
            } else {
                prev_state_port1 = 0;
                mqtt_publish("dev/ports/Port 1", "true");
            }


            break;

        // two

        case 0xE619:
            printf("2\n");

            if (repeat == 1) break;
            short_beep();
            if (prev_state_port2 == 0) {
                prev_state_port2 = 1;
                mqtt_publish("dev/ports/Port 2/port", "false");
            } else {
                prev_state_port2 = 0;
                mqtt_publish("ports/Port 2/port", "true");
            }
            break;

        // three

        case 0xF20D:
            printf("3\n");

            if (repeat == 1) break;
            short_beep();
            if (prev_state_port3 == 0) {
                prev_state_port3 = 1;
                mqtt_publish("dev/ports/Port 3/port", "false");
            } else {
                prev_state_port3 = 0;
                mqtt_publish("dev/ports/Port 3/port", "true");
            }

            break;

        // four

        case 0xF30C:
            printf("4\n");
            break;

        // five

        case 0xE718:
            printf("5\n");
            break;

        // six

        case 0xA15E:
            printf("6\n");
            break;


        case 0xF708:
            printf("7\n");
            break;
        
        case 0xE31C:
            printf("8\n");
            break;


        case 0xA55A:
            printf("9\n");
            break;  


        case 0xBD42:
            printf("*\n");
            break;
        

        case 0xAD52:
            printf("0\n");
            break;


        case 0xB54A:
            printf("#\n");
            // deactivating alarm
                gpio_set_level(ALARM_PIN, 0); 
                alarm_state = 0;
            
            break;
        

       default:
            printf("Unknown command\n");
            break;

    }

}
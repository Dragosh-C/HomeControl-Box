idf_component_register(
    SRCS "app_main.c"
         "mqtt.c"
         "sensors/sensor.c"
         "sensors/read_sensors.c"
         "sensors/configure_adc.c"
         "sensors/IR/ir_sensor.c"
         "sensors/IR/interpret_cmd.c"
         "output/led_signals.c"
         "wifi_setup.c"
    INCLUDE_DIRS "."
)

import machine
import ujson
import os
import network
CREDENTIALS_FILE = "/credentials.json" 

from acces_point_mode import connect_to_wifi
from acces_point_mode import start_server
import mqtt

access_point_button = machine.Pin(10, machine.Pin.IN, machine.Pin.PULL_UP)

def load_credentials():
    try:
        with open(CREDENTIALS_FILE, 'r') as f:
            credentials = ujson.load(f) 
        return credentials
    except OSError:
        print("No credentials file found, goind in AP mode.")
        start_server()

        return None
    except ValueError:
        print("Credentials file error.")
        return None

if not access_point_button.value():
    print('Button pressed')
    start_server()

else:

    print('Button released')
    credentials = load_credentials()
    
    if credentials:
        print("Credentials loaded from file:", credentials)

        ssid = credentials.get("ssid")
        password = credentials.get("password")
        mqtt_broker = credentials.get("mqtt_broker")

        print(f"Connecting to WiFi: {ssid} ...")
        connect_to_wifi(ssid, password, mqtt_broker)

        print("MQTT Broker:", mqtt_broker)

        if network.WLAN(network.STA_IF).isconnected():
            mqtt.configure_mqtt(mqtt_broker)




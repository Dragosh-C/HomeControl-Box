from umqtt.simple import MQTTClient
import machine
import time


def configure_mqtt(mqtt_broker):
    client_id = "outlet_1001"
    topic = "outlet/1001/relay"

    def sub_cb(topic, msg):
        print(f"Received message: {msg} on topic: {topic}")

        if msg == "on":
            print("Turning relay ON")
            machine.Pin(16, machine.Pin.OUT, value=0)
        elif msg == "off":
            print("Turning relay OFF")
            machine.Pin(16, machine.Pin.OUT, value=1)

    client = MQTTClient(client_id, mqtt_broker)
    client.set_callback(sub_cb)

    try:
        print(f"Trying to connect to MQTT broker at {mqtt_broker}...")
        client.connect()
        client.subscribe(topic)
        print("Connected to MQTT broker")

        while True:
            client.check_msg()

            # read data from adc and publish it to the broker ( power usage)

            power_usage = machine.ADC(0).read_u16()

            # client.publish("outlet/1001/power", str(power_usage))

            time.sleep(0.25)

    except IndexError as e:
        print(f"IndexError: {e}. Error mqtt connection.")
    except OSError as e:
        print(f"OSError: {e}. Connection failed.")
        time.sleep(5)

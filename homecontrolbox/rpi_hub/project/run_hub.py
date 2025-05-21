import paho.mqtt.client as mqtt
import ssl
import firebase_admin
from firebase_admin import credentials, db
import listen_task



# Firebase Configuration
SERVICE_ACCOUNT_KEY_PATH = '/home/pi/project/serviceAccountKey.json'
DATABASE_URL = 'https://smart-home-app-7c709-default-rtdb.europe-west1.firebasedatabase.app/'

cred = credentials.Certificate(SERVICE_ACCOUNT_KEY_PATH)
firebase_admin.initialize_app(cred, {
    'databaseURL': DATABASE_URL
})

# Write data to Firebase Realtime Database
def write_to_database(path, data):
    ref = db.reference(path)
    existing_data = ref.get()

    if existing_data == data:
        return

    ref.set(data)
    print(f"Data written to {path}: {data}")

# Variable to track dimmer value changes
dimmer_changed = False

# MQTT on_message callback
def on_message(client, userdata, msg):
    try:
        message = msg.payload.decode("utf-8")
        print(f"Received message: {message} on topic: {msg.topic}")

        if msg.topic == "dev/devices/ports/Port 1/port":
            path = msg.topic.removeprefix("dev/")
            data = True if message == "true" else False
            write_to_database(path, data)
            return

        elif msg.topic == "dev/devices/ports/Port 2/port":
            path = msg.topic.removeprefix("dev/")
            data = True if message == "true" else False
            write_to_database(path, data)
            return

        elif msg.topic == "dev/devices/ports/Port 3/port":
            path = msg.topic.removeprefix("dev/")
            data = True if message == "true" else False
            write_to_database(path, data)
            return

        elif msg.topic == "dev/outlet/1001/power":
            path = "devices/1001/power_usage"
            data = float(message)
            global div
            if div == 0:
                write_to_database(path, data)
                div = div + 1
            else:
                div = div + 1
            if div == 2:
                div = 0
            return

        data = int(message)
        topic = msg.topic.removeprefix("dev/")
        write_to_database(topic, data)

    except Exception as e:
        print(f"Error processing message: {e}")

# Monitor alarm changes in Firebase
def monitor_alarm_changes():
    ref = db.reference('/box_id/1212/alarm')

    def listener(event):
        alarm_status = event.data
        print(alarm_status)
        if alarm_status is not None:
            if alarm_status == "true":
                mqtt_client.publish("/box_id/1212/alarm", "on")
                print("Alarm activated, message published.")
            else:
                mqtt_client.publish("/box_id/1212/alarm", "off")
                print("Alarm deactivated, message published.")

    ref.listen(listener)

# Monitor dimmer changes (separate for Port 4 dimmer)
def monitor_dimmer():
    global dimmer_changed
    ref = db.reference('/devices/ports/Port 4/dimmer')

    def listener(event):
        dimmer_value = event.data
        print(f"Dimmer value changed: {dimmer_value}")  # Send data to listener
        
        # Set the dimmer_changed flag to True when dimmer value is updated
        dimmer_changed = True

        # Publish to the correct MQTT topic
        mqtt_client.publish("devices/ports/Port 4/dimmer", str(dimmer_value))
    
    ref.listen(listener)

# Monitor actuators and ports, ensuring dimmer changes are not mistaken for port changes
def monitor_all_actuators():
    global dimmer_changed
    ref = db.reference('/devices')

    def listener(event):
        global dimmer_changed
        path_parts = event.path.strip('/').split('/')

        # Skip changes if the event corresponds to a dimmer change
        if len(path_parts) >= 2 and path_parts[0] == 'ports' and path_parts[1] == 'Port 4' and path_parts[2] == 'dimmer':
            return  # Ignore dimmer updates here

        # Handle actuator updates
        if len(path_parts) >= 2 and path_parts[1] == "actuator" and not dimmer_changed:
            device_id = path_parts[0]  # Extract device ID
            actuator_status = event.data  # The new actuator value

            print(f"Device {device_id} actuator status changed: {actuator_status}")

            if actuator_status is not None:
                mqtt_message = "off" if actuator_status else "on"
                mqtt_client.publish(f"outlet/{device_id}/relay", mqtt_message)
                print(f"Message published to MQTT: {mqtt_message}")

        # Reset dimmer_changed flag after processing actuator logic
        if dimmer_changed:
            dimmer_changed = False

        # If the event corresponds to a port update (and not dimmer), publish port status
        if len(path_parts) >= 2 and path_parts[0] == "ports":
            if event.data == True:
                value = "true"
            else:
                value = "false"
            # Avoid sending Port 4/port update when dimmer is changed
            if path_parts[1] != "Port 4" or not dimmer_changed:
                mqtt_client.publish(f"devices/ports/{path_parts[1]}/port", value)
                print(f"Message published to MQTT: devices/ports/{path_parts[1]}/port: {value}")

    ref.listen(listener)

# Connect to MQTT Broker with Secure TLS Settings
def connect_mqtt(broker_address):
    client = mqtt.Client()
    client.on_message = on_message

    # Enable TLS
    client.tls_set(ca_certs="/home/pi/project/mosquitto/certs/ca.crt",
                   tls_version=ssl.PROTOCOL_TLSv1_2)

    # Set username and password if the broker requires it
    # client.username_pw_set(username="your_username", password="your_password")
    client.tls_insecure_set(True) 
    client.connect(broker_address, 8883, 60)

    # Subscribe to topics
    client.subscribe("dev/#")
    client.loop_start()
    return client

if __name__ == "__main__":
    MQTT_BROKER_ADDRESS = '192.168.0.101'
    print(f"Connecting to MQTT broker at {MQTT_BROKER_ADDRESS} using secure connection.")
    mqtt_client = connect_mqtt(MQTT_BROKER_ADDRESS)

    # Start monitoring
    monitor_alarm_changes()
    monitor_dimmer()
    monitor_all_actuators()

    # tasks = listen_task.get_routines()

    listen_task.monitor_routines()

    # tasks = listen_task.get_routines()
    tasks = listen_task.routines_store
    for task in tasks:
        print(f"Task: {task}")
        # Here you can add logic to execute the task
        # For example, you can call a function to execute the task
        # execute_task(task)

    listen_task.create_task()

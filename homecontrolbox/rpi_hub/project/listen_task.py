from firebase_admin import db
from dataclasses import dataclass
import threading
import time
from datetime import datetime, timedelta
import re

@dataclass
class Routine:
    id: int
    enabled: bool
    ifCondition: str
    repeatEveryDay: bool
    thenAction: str
    title: str
    type: str
    wasExecuted: bool = False
    timeWhenExecuted: str = ""
    device_id: str = None

# Holds all routines in memory
routines_store = {}

# Load all routines once at startup
def load_all_routines(path='/routines'):
    raw_data = db.reference(path).get()
    if raw_data:
        for key, val in raw_data.items():
            try:
                routine = Routine(
                    id=val.get("id", int(key)),
                    enabled=val.get("enabled", False),
                    ifCondition=val.get("ifCondition", ""),
                    repeatEveryDay=val.get("repeatEveryDay", False),
                    thenAction=val.get("thenAction", ""),
                    title=val.get("title", ""),
                    type=val.get("type", ""),
                    wasExecuted=val.get("wasExecuted", False),
                    timeWhenExecuted=val.get("timeWhenExecuted", ""),
                    device_id=val.get("deviceID", None)
                )
                routines_store[key] = routine
                print(f"Loaded routine {key}: {routine}")
            except Exception as e:
                print(f"Error loading routine {key}: {e}")

# Live listener for updates
def on_routines_update(event):
    path_parts = event.path.strip('/').split('/')
    routine_key = path_parts[0] if path_parts else None

    if not routine_key:
        return

    if event.data is None:
        # Routine deleted
        if routine_key in routines_store:
            del routines_store[routine_key]
            print(f"Routine {routine_key} deleted.")
        return

    try:
        # Partial update — fetch full data
        if len(path_parts) > 1:
            full_data = db.reference(f"/routines/{routine_key}").get()
        else:
            full_data = event.data

        if not isinstance(full_data, dict):
            print(f"Invalid format for routine {routine_key}: {full_data}")
            return

        routine = Routine(
            id=full_data.get("id", int(routine_key)),
            enabled=full_data.get("enabled", False),
            ifCondition=full_data.get("ifCondition", ""),
            repeatEveryDay=full_data.get("repeatEveryDay", False),
            thenAction=full_data.get("thenAction", ""),
            title=full_data.get("title", ""),
            type=full_data.get("type", ""),
            wasExecuted=full_data.get("wasExecuted", False),
            timeWhenExecuted=full_data.get("timeWhenExecuted", ""),
            device_id=full_data.get("deviceID", None)            
        )

        routines_store[routine_key] = routine
        print(f"Routine {routine_key} updated: {routine}")

    except Exception as e:
        print(f"Error processing routine {routine_key}: {e}")

# Combine loading and listening
def monitor_routines(path='/routines'):
    load_all_routines(path)        # Step 1: Initial full load
    ref = db.reference(path)
    ref.listen(on_routines_update)  # Step 2: Live updates


dimmer_path = '/devices/ports/Port 4/dimmer'
light_intensity = 0
dimmer_changed = False

def dimmer_listener(event):
    global dimmer_changed, light_intensity
    new_value = event.data
    if new_value is not None:
        print(f"Dimmer value changed: {new_value}")
        if int(new_value) != light_intensity:
            dimmer_changed = True


def alarm_task():
    global light_intensity, dimmer_changed
    light_intensity = 0
    dimmer_changed = False

    # Start Firebase listener in a thread
    ref = db.reference(dimmer_path)
    listener_thread = threading.Thread(target=ref.listen, args=(dimmer_listener,))
    listener_thread.daemon = True
    listener_thread.start()

    # Light intensity loop
    try:
        while light_intensity < 100 and not dimmer_changed:
            light_intensity += 5
            print(f"Increasing light to {light_intensity}%")
            db.reference(dimmer_path).set(light_intensity)
            time.sleep(60)
    except KeyboardInterrupt:
        print("Alarm task stopped.")

    
    print("Alarm task completed.")




def extract_condition_info(condition: str):
    """Extracts sensor type, operator, and threshold value."""
    match = re.search(r"(Temperature|Humidity|Luminosity|Power|Air Quality)\s*([<>=])\s*([\d.]+)", condition)
    if match:
        sensor_type, operator, value = match.groups()
        return sensor_type.strip(), operator, float(value)
    return None, None, None

def get_sensor_value(sensor_type, device_id):
    """Fetch the current sensor value from Firebase."""
    key_map = {
        "Temperature": "temperature",
        "Humidity": "humidity",
        "Luminosity": "light_intensity",
        "Power": "power_usage",
        "Air Quality": "air_quality"
    }

    print(f"Fetching {sensor_type} for device {device_id}")
    path = key_map.get(sensor_type)
    if not path:
        return None

    print(f"Path: {path}")
    return db.reference(f'/box_id/{device_id}/{path}').get()

def evaluate_condition(value, operator, threshold):
    """Evaluates the condition based on the operator."""
    if value is None:
        return False
    if operator == "<":
        return value < threshold
    elif operator == ">":
        return value > threshold
    elif operator == "=":
        return value == threshold
    return False

def execute_then_action(routine, key):
    """Executes the specified action."""
    if routine.thenAction.startswith("Port"):
        splited_str = routine.thenAction.split(" ")
        port_id = splited_str[0] + " " + splited_str[1]
        db.reference(f'/devices/ports/{port_id}/port').set(True)
        print(f"Relay activated on port {port_id}")

    elif routine.thenAction.startswith("Device with ID"):
        match = re.search(r"Device with ID (\d+)", routine.thenAction)
        if match:
            device_id = match.group(1)
            db.reference(f'/devices/{device_id}/actuator').set(True)
            print(f"Relay activated on device with ID {device_id}")










def create_task():
    """
    Create a task for the given routine.
    """
    
    global routines_store

    while True:

        for key, routine in routines_store.items():
            if routine.enabled:
                # Check if the condition is met
                    # if its midnight and repeatEveryDay is true
                    current_time = datetime.now().strftime("%H:%M")
                    if routine.repeatEveryDay and current_time == "00:00":
                        routine.wasExecuted = False
                        db.reference(f"/routines/{key}/wasExecuted").set(routine.wasExecuted)
                        print(f"Routine {key} reset at midnight.")

                    if routine.type == "Alarm":
                        if routine.ifCondition.startswith("Alarm set for"):
                            alarm_time_str = routine.ifCondition.split(" ")[-1]
                            now = datetime.now()
                            current_time_str = now.strftime("%H:%M")
                            
                            # Parse the alarm time
                            try:
                                alarm_time = datetime.strptime(alarm_time_str, "%H:%M")
                                alarm_time = alarm_time.replace(year=now.year, month=now.month, day=now.day)
                                
                                # Subtract 30 minutes
                                pre_alarm_time = alarm_time - timedelta(minutes=30)

                                if routine.repeatEveryDay:
                                    if alarm_time < now:
                                        routine.wasExecuted = False

                                # Trigger 30 minutes before actual alarm time
                                if now >= pre_alarm_time and not routine.wasExecuted and routine.enabled:
                                    print(f"Pre-alarm phase for {alarm_time_str} triggered at {current_time_str}")
                                    routine.wasExecuted = True
                                    routine.timeWhenExecuted = current_time_str
                                    db.reference(f"/routines/{key}/wasExecuted").set(routine.wasExecuted)
                                    db.reference(f"/routines/{key}/timeWhenExecuted").set(routine.timeWhenExecuted)

                                    if routine.thenAction == "Alarm triggered":
                                        threading.Thread(target=alarm_task, daemon=True).start()

                

                            except ValueError:
                                print(f"Invalid time format in routine: {alarm_time_str}")


                    # if routine.type == "Relay":
                    #     if routine.enabled and not routine.wasExecuted and routine.ifCondition.startswith("Temperature"):
                    #         if '<' in routine.ifCondition:
                    #             match = re.search(r"<\s*([\d.]+)", routine.ifCondition)
                    #             if match:
                    #                 temperature_threshold = float(match.group(1))
                    #             print(f"Temperature threshold: {temperature_threshold}")

                                
                    #             current_temperature = db.reference(f'/box_id/{routine.device_id}/temperature').get()

                    #             print(f"Current temperature: {current_temperature}")

                    #             # check current time
                    #             current_time = datetime.now().strftime("%H:%M")
                    #             if current_temperature is not None and current_temperature > temperature_threshold:
                    #                 print(f"Temperature alert! Current: {current_temperature}, Threshold: {temperature_threshold}")
                    #                 routine.wasExecuted = True
                    #                 routine.timeWhenExecuted = datetime.now().strftime("%H:%M")
                    #                 db.reference(f"/routines/{key}/wasExecuted").set(routine.wasExecuted)
                    #                 db.reference(f"/routines/{key}/timeWhenExecuted").set(routine.timeWhenExecuted)

                    #                 if routine.thenAction.startswith("Port"):
                    #                     splited_str = routine.thenAction.split(" ")

                    #                     port_id = splited_str[0] + " " + splited_str[1]
                    #                     print(f"Port ID: {port_id}")
                    #                     db.reference(f'/devices/ports/{port_id}/port').set(True)
                    #                     print(f"Relay activated on port {port_id}")
                                    
                    #                 if routine.thenAction.startswith("Device with ID"):
                    #                     # regex to extract device ID
                    #                     match = re.search(r"Device with ID (\d+)", routine.thenAction)
                    #                     if match:
                    #                         device_id = match.group(1)
                    #                         print(f"Device ID: {device_id}")
                    #                         db.reference(f'/devices/{device_id}/actuator').set(True)
                    #                         print(f"Relay activated on device with ID {device_id}")
                                    
                    if routine.type == "Relay":
                        if routine.enabled and not routine.wasExecuted:
                            sensor_type, operator, threshold = extract_condition_info(routine.ifCondition)

                            if sensor_type and operator and threshold is not None:
                                current_value = get_sensor_value(sensor_type, routine.device_id)
                                print(f"{sensor_type} current value: {current_value}, Threshold: {operator} {threshold}")

                                if evaluate_condition(current_value, operator, threshold):
                                    print(f"{sensor_type} alert! Current: {current_value}, Condition met: {operator} {threshold}")
                                    routine.wasExecuted = True
                                    routine.timeWhenExecuted = datetime.now().strftime("%H:%M")

                                    db.reference(f"/routines/{key}/wasExecuted").set(routine.wasExecuted)
                                    db.reference(f"/routines/{key}/timeWhenExecuted").set(routine.timeWhenExecuted)

                                    execute_then_action(routine, key)

        time.sleep(5)

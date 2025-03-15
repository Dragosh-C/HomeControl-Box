import network
import machine
import socket
import ure
import time
import ujson

ap = network.WLAN(network.AP_IF)
ap.active(True)
ap.config(essid="ESP32-Setup", password="12345678")

CREDENTIALS_FILE = "/credentials.json" 

def save_credentials(ssid, password, mqtt_broker):
    credentials = {
        "ssid": ssid,
        "password": password,
        "mqtt_broker": mqtt_broker
    }
    with open(CREDENTIALS_FILE, 'w') as f:
        ujson.dump(credentials, f)


html = """
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 WiFi & MQTT Config</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        .container {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1);
            width: 90%;
            max-width: 300px;
            text-align: center;
        }
        input {
            width: calc(100% - 20px);
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ccc;
            border-radius: 5px;
            font-size: 16px;
            box-sizing: border-box;
        }
        button {
            width: 100%;
            padding: 10px;
            background: #007bff;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
        }
        button:hover {
            background: #0056b3;
        }
    </style>
</head>
<body>
    <div class='container'>
        <h2>ESP32 Setup</h2>
        <form action='/' method='post'>
            <input type='text' name='ssid' placeholder='WiFi SSID' required>
            <input type='password' name='password' placeholder='WiFi Password' required>
            <input type='text' name='mqtt' placeholder='MQTT Broker Address' required>
            <button type='submit'>Save & Connect</button>
        </form>
    </div>
</body>
</html>
"""

def start_server():
    addr = ('0.0.0.0', 80)
    s = socket.socket()
    s.bind(addr)
    s.listen(5)
    print("Web server started on 192.168.4.1")

    while True:
        conn, addr = s.accept()
        request = conn.recv(1024).decode()
        print("Request:", request)
        
        if "POST" in request:
            try:
                form_data = request.split("\r\n\r\n")[-1]
                params = {}
                for param in form_data.split("&"):
                    key, value = param.split("=")
                    params[key] = value.replace("+", " ")  # Convert URL encoding

                ssid = params.get("ssid", "").strip()
                password = params.get("password", "").strip()
                mqtt_broker = params.get("mqtt", "").strip()

                print(f"SSID: {ssid}, Password: {password}, MQTT: {mqtt_broker}")

                if ssid and password:
                    connect_to_wifi(ssid, password, mqtt_broker)

            except Exception as e:
                print("Error processing form data:", e)

        conn.send("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n")
        conn.send(html)
        conn.close()

def connect_to_wifi(ssid, password, mqtt_broker):
    print(f"Trying to connect to WiFi: {ssid} ...")
    sta = network.WLAN(network.STA_IF)
    sta.active(True)
    sta.connect(ssid, password)
    
    for _ in range(10):  # Wait up to 10 seconds
        if sta.isconnected():
            print("Connected to WiFi!", sta.ifconfig())
            save_credentials(ssid, password, mqtt_broker)
            return
        time.sleep(1)
    print("Failed to connect to WiFi!")
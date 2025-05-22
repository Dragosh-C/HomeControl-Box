
# import subprocess
# import time
# import requests
# import os
# import signal

# # Start ngrok in the background
# ngrok_process = subprocess.Popen(
#     ["ngrok", "http", "192.168.0.102:80"],
#     stdout=subprocess.DEVNULL,
#     stderr=subprocess.STDOUT
# )

# # Wait a few seconds for ngrok to initialize
# time.sleep(5)

# # Query the ngrok API to get the public URL
# def get_ngrok_url():
#     try:
#         res = requests.get("http://127.0.0.1:4040/api/tunnels")
#         tunnels = res.json()["tunnels"]
#         for t in tunnels:
#             if t["proto"] == "https":
#                 return t["public_url"]
#     except Exception as e:
#         print("Error getting ngrok URL:", e)
#     return None

# public_url = get_ngrok_url()
# if public_url:
#     print("Ngrok public URL:", public_url)
# else:
#     print("Ngrok tunnel not found.")

# # Optional: stop ngrok after you're done
# # ngrok_process.send_signal(signal.SIGTERM)

import subprocess
import time
import requests
import firebase_admin
from firebase_admin import credentials, db

# Firebase Configuration
SERVICE_ACCOUNT_KEY_PATH = '/home/pi/project/serviceAccountKey.json'
DATABASE_URL = 'https://smart-home-app-7c709-default-rtdb.europe-west1.firebasedatabase.app/'

cred = credentials.Certificate(SERVICE_ACCOUNT_KEY_PATH)
firebase_admin.initialize_app(cred, {
    'databaseURL': DATABASE_URL
})

# Start ngrok in the background
ngrok_process = subprocess.Popen(
    ["ngrok", "http", "192.168.0.102:80"],
    stdout=subprocess.DEVNULL,
    stderr=subprocess.STDOUT
)

time.sleep(20)  # wait for ngrok to start

def get_ngrok_url():
    try:
        res = requests.get("http://127.0.0.1:4040/api/tunnels")
        tunnels = res.json()["tunnels"]
        for t in tunnels:
            if t["proto"] == "https":
                return t["public_url"]
    except Exception as e:
        print("Error getting ngrok URL:", e)
    return None

public_url = get_ngrok_url()

if public_url:
    print("Ngrok public URL:", public_url)
    ref = db.reference('/camera/1001')
    ref.update({'ngrok_url': public_url})
    print("Ngrok URL pushed to Firebase under /camera/1001/ngrok_url")
else:
    print("Ngrok tunnel not found.")


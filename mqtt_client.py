import paho.mqtt.client as paho
import time

# https://os.mbed.com/teams/mqtt/wiki/Using-MQTT#python-client

# MQTT broker hosted on local machine
mqttc = paho.Client()

# Settings for connection
# TODO: revise host to your IP
host = "172.20.10.2"
topic = "Mbed"
c = 0
# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    #print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n")
    minusb = str(msg.payload).split("'") 
    minus_slash = minusb[1].split("\\",1) 
    if (str(minus_slash[0]) == "111000.000000"):
        print("Avoidance!!")
    elif (str(minus_slash[0]) == "1100.000000"):
        print("go straight!!")
    elif (str(minus_slash[0]) == "1110.000000"):
        print("go back!!")
    else:
        print("Distance: " + str(minus_slash[0]) + "cm")

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed OK")

def on_unsubscribe(mosq, obj, mid, granted_qos):
    print("Unsubscribed OK")

# Set callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

# Connect and subscribe
print("Connecting to " + host + "/" + topic)
mqttc.connect(host, port=1883, keepalive=60)
mqttc.subscribe(topic, 0)

# Publish messages from Python
num = 0

# Loop forever, receiving messages
mqttc.loop_forever()
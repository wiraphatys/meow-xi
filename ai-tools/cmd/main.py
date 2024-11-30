
import cv2
import paho.mqtt.client as mqtt
from ultralytics import YOLO
import time

# NETPIE MQTT credentials
MQTT_BROKER = "mqtt.netpie.io"
MQTT_PORT = 1883
MQTT_USERNAME = "eU7e3SsAiB9jKG6rF9cf2SvVJLN9KEBS"
MQTT_PASSWORD = "NPn2bf9Eh7eZTEEzSSZD8CXCYzRoMP18"
MQTT_CLIENT_ID = "2eb9e658-1463-401c-bbb9-7a626b3d3022"
SUBSCRIBE_TOPIC = "@msg/message01"
PUBLISH_TOPIC = "@msg/ai/detect/cat"

# Initialize YOLO model
model = YOLO('./ai-tools/model/yolo11n.pt')  # Change to the path of your YOLO model


class MQTTClient:
    def __init__(self, broker, port, username, password, client_id):
        self.client = mqtt.Client(client_id)
        self.client.username_pw_set(username, password)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(broker, port, 60)
        self.client.loop_start()

    def on_connect(self, client, userdata, flags, rc):
        print(f"Connected with result code {rc}")
        self.client.subscribe(SUBSCRIBE_TOPIC)  # Subscribe to the topic

    def on_message(self, client, userdata, msg):
        print(f"Message received: {msg.payload.decode()}")

    def publish(self, topic, message):
        print(f"Publishing message to topic {topic}: {message}")
        self.client.publish(topic, message)


class ObjectDetection:
    def __init__(self, mqtt_client: MQTTClient):
        self.cap = cv2.VideoCapture(1)
        self.mqtt_client = mqtt_client
        self.last_published = time.time()  # Track last time message was published

        if not self.cap.isOpened():
            print("Error: Could not open webcam.")
            exit()

    def detect_objects(self, frame):
        # Run detection using YOLO
        results = model(frame)
        annotated_frame = results[0].plot()  # Annotate frame with results

        # Check if a cat is detected
        detected_objects_list = [
            model.names[int(cls)] for cls in results[0].boxes.cls]
        detected_objects_set = set(detected_objects_list)

        # If 'cat' is detected, publish "true" only if it hasn't been published recently
        if "cat" in detected_objects_set and time.time() - self.last_published >= 2:  # 2 second cooldown
            print("Cat detected!")
            self.mqtt_client.publish(PUBLISH_TOPIC, "true")
            self.last_published = time.time()
        elif "cat" not in detected_objects_set and time.time() - self.last_published >= 2:
            # Publish "false" only if no cat detected and we haven't sent "false" recently
            print("No cat detected.")
            self.mqtt_client.publish(PUBLISH_TOPIC, "false")
            self.last_published = time.time()

        return annotated_frame

    def run(self):
        while True:
            ret, frame = self.cap.read()
            if not ret:
                print("Error: Could not read frame.")
                break

            # Detect objects and annotate the frame
            annotated_frame = self.detect_objects(frame)

            # Display the annotated frame
            cv2.imshow('YOLOv11 Real-Time Detection', annotated_frame)

            # Exit when 'q' is pressed
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        # Release the capture and close any OpenCV windows
        self.cap.release()
        cv2.destroyAllWindows()


def main():
    # Set up MQTT Client
    mqtt_client = MQTTClient(MQTT_BROKER, MQTT_PORT,
                             MQTT_USERNAME, MQTT_PASSWORD, MQTT_CLIENT_ID)

    # Set up Object Detection and start real-time video feed
    object_detection = ObjectDetection(mqtt_client)
    object_detection.run()


if __name__ == "__main__":
    main()

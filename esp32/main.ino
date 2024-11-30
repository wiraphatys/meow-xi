#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <GP2YDustSensor.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char *ssid = "REPLACE_WITH_YOUR_WIFI_NAME";
const char *password = "REPLACE_WITH_YOUR_WIFI_PASSWORD";

// NETPIE credentials
const char *mqtt_server = "mqtt.netpie.io";
const char *mqtt_client_id = "009fd8f0-e42b-446a-a816-21d4a21bbe92";
const char *mqtt_username = "zcGice2xpnhbU5vDWpzwBfKDX9FVoS1y";
const char *mqtt_password = "ToL8eM1ABWPvkrBs9V9ETJKAPiXiKycM";

// MQTT topics
const char *publish_topic = "@shadow/data/update";
const char *subscribe_topic = "@msg/ai/detect/cat";

// Telegram Bot credentials
const String TELEGRAM_TOKEN = "7585799707:AAHPhh-sU74zyQC_ghNl2bDxNkLEiS03hXU";
const String TELEGRAM_CHAT_ID = "2025463581";

// DHT22 settings
#define DHTPIN 19         // Pin where DHT22 is connected
#define DHTTYPE DHT22     // Define sensor type
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// GP2Y1014AU0F settings
const uint8_t SHARP_LED_PIN = 25; // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = 33;  // Sharp Dust/particle analog out pin used for reading
const float V_LED = 0.6;
const float DUST_DENSITY_CONSTANT = 0.5;
GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1014AU0F, SHARP_LED_PIN, SHARP_VO_PIN);

// Buzzer settings
#define BUZZER_PIN 4 // Pin where Buzzer is connected

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Callback function to handle incoming MQTT messages
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    String receivedMessage = "";
    for (unsigned int i = 0; i < length; i++)
    {
        receivedMessage += (char)payload[i];
    }
    Serial.println(receivedMessage);

    // If receivedMessage is "true", trigger buzzer for a short sound
    if (receivedMessage == "true")
    {
        String messageToSend = "🚨ALERT🚨 Your cat has escaped from the house! 🐾";

        sendTelegramAlert(messageToSend);
        detectedCatSoundAlert();
    }
}

void setup_wifi()
{
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 15000; // 15-second timeout for Wi-Fi connection

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout)
    {
        delay(5000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWi-Fi connected.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWi-Fi connection failed. Restarting...");
        ESP.restart(); // Restart the ESP to retry
    }
}

void reconnect()
{
    static unsigned long lastAttemptTime = 0;
    const unsigned long retryInterval = 5000; // Retry every 5 seconds

    if (millis() - lastAttemptTime > retryInterval)
    {
        lastAttemptTime = millis();
        Serial.print("Attempting MQTT connection...");
        if (client.connect(mqtt_client_id, mqtt_username, mqtt_password))
        {
            Serial.println("Connected to MQTT broker.");
            client.subscribe(subscribe_topic);
        }
        else
        {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying...");
        }
    }
}

float getDustDensity()
{
    return dustSensor.getDustDensity() / 10.0;
}

void sendTelegramAlert(String message)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        // Telegram API endpoint for sending messages
        String url = "https://api.telegram.org/bot" + TELEGRAM_TOKEN + "/sendMessage";

        // Prepare the HTTP request payload
        String payload = "chat_id=" + TELEGRAM_CHAT_ID + "&text=" + message;

        // Start HTTP request
        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Send HTTP POST request
        int httpCode = http.POST(payload);

        // Check the response
        if (httpCode > 0)
        {
            String response = http.getString();
            Serial.println("Telegram response: " + response);
        }
        else
        {
            Serial.println("Error sending message: " + String(httpCode));
        }

        // End the HTTP request
        http.end();
    }
    else
    {
        Serial.println("WiFi not connected");
    }
}

void detectedCatSoundAlert()
{
    tone(BUZZER_PIN, 1000); // 1 kHz
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 500); // 0.5 kHz
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 800); // 0.8 kHz
    delay(250);
    noTone(BUZZER_PIN);
    delay(100);
}

void envOutOfRangeSoundAlert()
{
    // Create a fun, energetic alert sound
    tone(BUZZER_PIN, 1000); // 1 kHz
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 1500); // 1.5 kHz
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 2000); // 2 kHz
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 2500); // 2.5 kHz
    delay(300);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 2000); // 2 kHz (lower)
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 1500); // 1.5 kHz
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);

    tone(BUZZER_PIN, 1000); // 1 kHz (low)
    delay(300);
    noTone(BUZZER_PIN);
}

void checkEnvQualityAndAlert(float temperature, float humidity, float dustDensity)
{
    String alertMessage = "⚠️ Environmental Alert! ⚠️\n";
    bool alertTriggered = false;

    // Check for temperature out of range
    if (temperature < 10 || temperature > 40)
    {
        alertMessage += "❄️ Temperature is out of range! (";
        alertMessage += temperature;
        alertMessage += "°C)\n";
        alertTriggered = true;
    }

    // Check for humidity out of range
    if (humidity < 30 || humidity > 70)
    {
        alertMessage += "💧 Humidity is out of range! (";
        alertMessage += humidity;
        alertMessage += "%)\n";
        alertTriggered = true;
    }

    // Check for dust density out of range
    if (dustDensity > 100)
    {
        alertMessage += "🌫️ Dust density is too high! (";
        alertMessage += dustDensity;
        alertMessage += " µg/m³)\n";
        alertTriggered = true;
    }

    // If any alert condition triggered, send Telegram message
    if (alertTriggered)
    {
        sendTelegramAlert(alertMessage);
        envOutOfRangeSoundAlert();
    }
}

void setup()
{
    Serial.begin(115200);

    // Initialize DHT sensor
    dht.begin();

    // Initialize GP2Y1014AU0F sensor
    dustSensor.begin();

    // Initialize Buzzer pin
    tone(BUZZER_PIN, 1000);
    delay(100);
    noTone(BUZZER_PIN);

    // Connect to Wi-Fi
    setup_wifi();

    // Setup MQTT client
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

unsigned long lastSensorReadTime = 0;
const unsigned long READ_INTERVAL = 5000;

void loop()
{
    // Reconnect to Wi-Fi if disconnected
    if (WiFi.status() != WL_CONNECTED)
    {
        setup_wifi();
    }

    // Ensure MQTT connection
    if (!client.connected())
    {
        reconnect();
    }
    client.loop(); // Allow MQTT client to process incoming messages

    if (millis() - lastSensorReadTime >= READ_INTERVAL)
    {
        // Set sensor read time
        lastSensorReadTime = millis();
        // Read DHT sensor data
        float temperature = dht.readTemperature(); // Celsius
        float humidity = dht.readHumidity();

        // Read GP2Y1014AU0F sensor data
        float dustDensity = getDustDensity();

        if (isnan(temperature) || isnan(humidity))
        {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }

        if (isnan(dustDensity))
        {
            Serial.println("Failed to read from GP2Y1014AU0F sensor!");
            return;
        }

        // Check if buzzer needs to be activated
        checkEnvQualityAndAlert(temperature, humidity, dustDensity);

        // Create JSON message with "data" wrapper
        String message = "{\"data\":{\"temperature\":";
        message += temperature;
        message += ",\"humidity\":";
        message += humidity;
        message += ",\"dust_density\":";
        message += dustDensity;
        message += "}}";

        if (!client.publish(publish_topic, message.c_str()))
        {
            Serial.println("Failed to publish MQTT message.");
        }
        Serial.print("Publishing message to NETPIE: ");
        Serial.println(message);
    }

    delay(500);
}

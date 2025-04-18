#include <WiFi.h>


/* ------------------- VARIABLES ------------------- */

const char* ssid = "ESP32_Host";  // Host's SSID
const char* password = "12345678";  // Host password
const char* hostIP = "192.168.4.1";  // Default SoftAP IP for the host
const int port = 8080;  // Port for communication
const int sensorPin = 34;  // Pressure sensor pin
const int relayPin = 12;  // Relay pin
const int threshold = 150;  // Threshold for relay activation

WiFiServer server(port);  // Server to handle incoming client connections
WiFiClient client;  // Client for connecting to the host

/* ------------------- SETUP ------------------- */

void setup() {
    Serial.begin(115200);
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);  // Make sure relay is off at startup

    // Try to connect as a client first by scanning for the host
    WiFi.mode(WIFI_STA);  // Set Wi-Fi mode to station (client)
    Serial.println("Scanning for Host...");

    int numNetworks = WiFi.scanNetworks();  // Scan for available networks
    bool hostFound = false;

    // Look for the host's SSID in the scanned networks
    for (int i = 0; i < numNetworks; i++) {
        if (WiFi.SSID(i) == ssid) {
            hostFound = true;
            break;
        }
    }

    if (hostFound) {
        Serial.println("Host found! Connecting as client...");
        WiFi.begin(ssid, password);  // Connect to the Host's AP

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nConnected to Host!");

        // Connect to the host server
        if (client.connect(hostIP, port)) {
            Serial.println("Connected to Host Server.");
        } else {
            Serial.println("Connection to Host failed.");
        }
    } else {
        Serial.println("No host found. Setting up as server...");
        // Set the ESP32 as an Access Point (Server)
        WiFi.softAP(ssid, password);
        Serial.print("Server IP: ");
        Serial.println(WiFi.softAPIP());

        server.begin();  // Start the server
    }
}

/* ------------------- LOOP ------------------- */

void loop() {
    // Check if in client mode
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
        // Client side code: receive sensor data and control relay
        if (client.available()) {
            String sensorData = client.readStringUntil('\n');  // Read the sensor data
            Serial.print("Received sensor data: ");
            Serial.println(sensorData);

            int pressureValue = sensorData.toInt();  // Convert to integer

            if (pressureValue >= threshold) {
                Serial.println("Threshold exceeded! Turning relay ON.");
                digitalWrite(relayPin, HIGH);  // Turn relay on
            } else {
                Serial.println("Below threshold. Relay OFF.");
                digitalWrite(relayPin, LOW);  // Turn relay off
            }
        }
    }
    
    // Check if in server mode (waiting for a client connection)
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        WiFiClient newClient = server.available();  // Check if a client is trying to connect
        if (newClient) {
            Serial.println("Client connected.");
            client = newClient;  // Update the client object with the new client
        }
    }
    
    // Server side: send sensor data to the client
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
        int sensorValue = analogRead(sensorPin);  // Read sensor data
        Serial.print("Sending sensor data: ");
        Serial.println(sensorValue);

        client.write(sensorValue);  // Send data to the client
        client.write("\n");         // Send end of line.

        delay(500);  // Send data every 500ms
    }
    
    // If client is disconnected, stop the connection
    if (!client.connected()) {
        Serial.println("Client disconnected.");
        client.stop();
    }
}

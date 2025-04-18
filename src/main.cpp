#include <WiFi.h>

const char* ssid = "ESP32_Host";  // WiFi SSID for the SoftAP
const char* password = "12345678";  // WiFi password (min. 8 characters)

WiFiServer server(8080);  // Create a TCP server on port 8080
const int sensorPin = 34;  // Analog pin for the pressure sensor

void setup() {
    Serial.begin(115200);
    
    // Set up ESP32 as an Access Point
    WiFi.softAP(ssid, password);
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());

    server.begin();  // Start TCP server
}

void loop() {
    WiFiClient client = server.available();  // Check for client connection
    
    if (client) {
        Serial.println("Client connected.");
        while (client.connected()) {
            int sensorValue = analogRead(sensorPin);  // Read pressure sensor
            
            Serial.print("Sending data: ");
            Serial.println(sensorValue);

            client.print(sensorValue);  // Send data to client
            client.print("\n");  // Ensure each value is on a new line

            String data = client.readStringUntil('\n');
            Serial.println(data);
            
            delay(500);  // Send data every 500ms
        }
        Serial.println("Client disconnected.");
        client.stop();  // Close the connection
    }
}
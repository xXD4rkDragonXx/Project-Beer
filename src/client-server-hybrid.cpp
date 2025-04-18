#include <WiFi.h>

// Credentials
const char* ssid = "ESP32_Host";
const char* password = "12345678";

// TCP settings
const int port = 8080;
const char* peerIP = "192.168.4.1";

// Pins
const int sensorPin = 34;
const int relayPin = 12;
const int threshold = 150;

// Networking
WiFiServer server(port);
WiFiClient client;     // TCP client (outgoing)
WiFiClient peerClient; // TCP server client (incoming)

bool isHostMode = true;

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000;

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Start SoftAP first (host mode)
  WiFi.softAP(ssid, password);
  Serial.print("SoftAP started, IP: ");
  Serial.println(WiFi.softAPIP());

  server.begin();

  // Simultaneously try to connect as a client
  WiFi.begin(ssid, password);
  attemptClientConnection();
}

void loop() {
  // Check client reconnection
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      Serial.println("Attempting to reconnect to peer...");
      client.stop();
      attemptClientConnection();
    }
  }

  // === Host Role ===
  if (isHostMode) {
    if (!peerClient || !peerClient.connected()) {
      peerClient = server.available();
    }

    if (peerClient && peerClient.connected()) {
      int sensorValue = analogRead(sensorPin);
      Serial.print("Sending sensor value: ");
      Serial.println(sensorValue);
      peerClient.print(sensorValue);
      peerClient.print("\n");
      delay(500);
    }
  }

  // === Client Role ===
  if (client.connected() && client.available()) {
    String sensorData = client.readStringUntil('\n');
    int pressure = sensorData.toInt();

    Serial.print("Received from peer: ");
    Serial.println(pressure);

    if (pressure >= threshold) {
      Serial.println("Threshold exceeded! Relay ON.");
      digitalWrite(relayPin, HIGH);
    } else {
      Serial.println("Below threshold. Relay OFF.");
      digitalWrite(relayPin, LOW);
    }
  }

  delay(100);
}

void attemptClientConnection() {
  Serial.print("Connecting to peer at ");
  Serial.println(peerIP);
  if (client.connect(peerIP, port)) {
    Serial.println("Client connected to peer's server.");
    isHostMode = false; // Prioritize client mode if connected
    server.end();
  } else {
    Serial.println("Client connection failed. Remaining in host mode.");
    isHostMode = true;
  }
}

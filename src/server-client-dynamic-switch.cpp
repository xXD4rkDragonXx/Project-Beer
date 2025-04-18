#include <WiFi.h>

/* ------------------- CONFIG ------------------- */

const char* ssid = "ESP32_Host";
const char* password = "12345678";
const char* hostIP = "192.168.4.1";
const int port = 8080;

const int sensorPin = 34;
const int relayPin = 12;
const int threshold = 150;
const unsigned long sensorInterval = 500;

/* ------------------- GLOBALS ------------------- */

WiFiServer server(port);
WiFiClient client;

bool clientConnected = false;
unsigned long lastSensorMillis = 0;

/* ------------------- FUNCTIONS ------------------- */

void connectToWiFiAsClient() {
  WiFi.mode(WIFI_STA);
  Serial.println("Scanning for Host...");

  bool hostFound = false;
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    if (WiFi.SSID(i) == ssid) {
      hostFound = true;
      break;
    }
  }

  if (hostFound) {
    Serial.println("Host found! Connecting as client...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);  // setup only
      Serial.print(".");
    }

    Serial.println("\nConnected to Host!");

    if (client.connect(hostIP, port)) {
      Serial.println("Connected to Host Server.");
      clientConnected = true;
    } else {
      Serial.println("Connection to Host failed.");
    }
  } else {
    startAsServer();
  }
}

void startAsServer() {
  Serial.println("No host found. Setting up as server...");
  WiFi.softAP(ssid, password);
  Serial.print("Server IP: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void sendSensorData() {
  unsigned long now = millis();
  if (now - lastSensorMillis >= sensorInterval) {
    int sensorValue = analogRead(sensorPin);
    Serial.print("Sending sensor data: ");
    Serial.println(sensorValue);

    client.print(sensorValue);
    client.print("\n");

    lastSensorMillis = now;
  }
}

void receiveAndControlRelay() {
  while (client.available()) {
    String sensorData = client.readStringUntil('\n');
    int pressure = sensorData.toInt();

    Serial.print("Received sensor value: ");
    Serial.println(pressure);

    digitalWrite(relayPin, pressure >= threshold ? HIGH : LOW);
    Serial.println(pressure >= threshold ? "Relay ON" : "Relay OFF");
  }
}

void checkForNewClient() {
  if (!clientConnected && WiFi.status() == WL_CONNECTED) {
    WiFiClient newClient = server.available();
    if (newClient) {
      Serial.println("Client connected.");
      client = newClient;
      clientConnected = true;
    }
  }
}

void checkClientConnection() {
  if (clientConnected && !client.connected()) {
    Serial.println("Client disconnected.");
    client.stop();
    clientConnected = false;
  }
}

/* ------------------- SETUP ------------------- */

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  connectToWiFiAsClient();
}

/* ------------------- LOOP ------------------- */

void loop() {
  if (clientConnected) {
    sendSensorData();         // Send your own sensor data
    receiveAndControlRelay(); // Act on received data
  }

  checkForNewClient();        // Accept new connections (if server)
  checkClientConnection();    // Clean up disconnected client
}

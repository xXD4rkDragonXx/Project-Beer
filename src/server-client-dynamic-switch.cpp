#include <WiFi.h>

/* ------------------- CONFIG ------------------- */

const char* ssid = "ESP32_Host";
const char* password = "12345678";
const char* hostIP = "192.168.4.1";
const int port = 8080;

const int sensorPin = 34;
const int buttonPin = 14;
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

void sendSensorAndButton() {
  unsigned long now = millis();
  if (now - lastSensorMillis >= sensorInterval) {
    int sensorValue = analogRead(sensorPin);
    int buttonState = digitalRead(buttonPin);

    Serial.print("Sending -> Sensor: ");
    Serial.print(sensorValue);
    Serial.print(", Button: ");
    Serial.println(buttonState);

    // Format: "sensor,button\n"
    client.printf("%d,%d\n", sensorValue, buttonState);

    lastSensorMillis = now;
  }
}

void receiveAndControlRelay() {
  while (client.available()) {
    String dataLine = client.readStringUntil('\n');
    int sepIndex = dataLine.indexOf(',');

    if (sepIndex > 0) {
      int remoteSensor = dataLine.substring(0, sepIndex).toInt();
      int remoteButton = dataLine.substring(sepIndex + 1).toInt();

      Serial.print("Received -> Sensor: ");
      Serial.print(remoteSensor);
      Serial.print(", Button: ");
      Serial.println(remoteButton);

      // Relay logic based on remote sensor
      digitalWrite(relayPin, remoteSensor >= threshold ? HIGH : LOW);
      Serial.println(remoteSensor >= threshold ? "Relay ON" : "Relay OFF");
    } else {
      Serial.println("Invalid data format received.");
    }
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
  pinMode(buttonPin, INPUT_PULLUP);  // Adjust as needed
  digitalWrite(relayPin, LOW);

  connectToWiFiAsClient();
}

/* ------------------- LOOP ------------------- */

void loop() {
  if (clientConnected) {
    sendSensorAndButton();      // Send sensor + button state
    receiveAndControlRelay();   // Act on remote sensor data
  }

  checkForNewClient();
  checkClientConnection();
}

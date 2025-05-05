#include <WiFi.h>

/*****************************************************************
 * This the the most complete version of the code.               *
 * It allows for dynamic switching between host and client.      *
 * Added logic for vibration motor when remote button is pressed *
 *****************************************************************/

/* ------------------- CONFIG ------------------- */

const char* ssid = "ESP32_Host";
const char* password = "12345678";
const int port = 8080;

const int sensorPin = 34;
const int buttonPin = 23;
const int relayPin = 16;
const int vibrationPin = 4;
const int ledPin = 2;  // Built-in LED

const int threshold = 150;
const unsigned long sensorInterval = 500;
const unsigned long blinkLong = 800;
const int retries = 5; 

/* ------------------- GLOBALS ------------------- */

WiFiServer server(port);
WiFiClient client;

bool clientConnected = false;
unsigned long lastSensorMillis = 0;
unsigned long lastBlink = 0;
bool ledState = false;

enum ConnectionState { SERVER_MODE, CONNECTED };
ConnectionState currentState = SERVER_MODE;

/* ------------------- FUNCTIONS ------------------- */

void handleLEDStatus() {
  unsigned long now = millis();

  if (currentState == CONNECTED) {
    digitalWrite(ledPin, HIGH);
  } else {
    // Only blink if in server mode
    if (now - lastBlink >= blinkLong) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState ? HIGH : LOW);
      lastBlink = now;
    }
  }
}

void startAsServer() {
  Serial.println("Starting as Server...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(500);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
  currentState = SERVER_MODE;
}

void attemptAutoPairing() {
  WiFi.mode(WIFI_STA);
  bool connected = false;

  for (int attempt = 0; attempt < retries && !connected; ++attempt) {
    Serial.printf("Client Attempt #%d to find host...\n", attempt + 1);

    int numNetworks = WiFi.scanNetworks();
    bool hostFound = false;
    for (int i = 0; i < numNetworks; ++i) {
      if (WiFi.SSID(i) == ssid) {
        hostFound = true;
        break;
      }
    }

    if (hostFound) {
      Serial.println("Host found! Connecting as client...");
      WiFi.begin(ssid, password);
      unsigned long startAttempt = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(50); // Short delay to prevent overload
      }

      if (WiFi.status() == WL_CONNECTED) {
        delay(1000);
        if (client.connect(WiFi.gatewayIP(), port)) {
          Serial.println("\nConnected to host server as client!");
          clientConnected = true;
          currentState = CONNECTED;
          return;
        } else {
          Serial.println("\nFailed to connect to host socket.");
          WiFi.disconnect();
        }
      } else {
        Serial.println("\nWiFi connection to host failed.");
      }
    } else {
      Serial.println("No host SSID found.");
    }

    delay(1000);
  }

  startAsServer();
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

      digitalWrite(relayPin, remoteSensor >= threshold ? HIGH : LOW);
      Serial.println(remoteSensor >= threshold ? "Relay ON" : "Relay OFF");

      digitalWrite(vibrationPin, remoteButton == LOW ? HIGH : LOW);
      Serial.println(remoteButton == LOW ? "Vibration ON" : "Vibration OFF");
    } else {
      Serial.println("Invalid data format received.");
    }
  }
}

void checkForNewClient() {
  if (!clientConnected) {
    WiFiClient newClient = server.available();
    if (newClient) {
      Serial.println("Client connected.");
      client = newClient;
      clientConnected = true;
      currentState = CONNECTED;
    }
  }
}

void checkClientConnection() {
  if (clientConnected && !client.connected()) {
    Serial.println("Client disconnected.");
    client.stop();
    clientConnected = false;
    currentState = SERVER_MODE;
  }
}

/* ------------------- SETUP ------------------- */

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(vibrationPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(vibrationPin, LOW);
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, LOW);

  attemptAutoPairing();
}

/* ------------------- LOOP ------------------- */

void loop() {
  if (clientConnected) {
    sendSensorAndButton();
    receiveAndControlRelay();
  }

  checkForNewClient();
  checkClientConnection();

  static unsigned long lastRetry = millis();
  if (!clientConnected && WiFi.getMode() == WIFI_AP && millis() - lastRetry > 15000) {
    Serial.println("Retrying auto-pairing...");
    WiFi.softAPdisconnect(true);
    attemptAutoPairing();
    lastRetry = millis();
  }

  handleLEDStatus();
}

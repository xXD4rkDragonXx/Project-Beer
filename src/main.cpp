#include <WiFi.h>

// Replace with your network credentials
const char* ssid     = "Project-Beer2";
const char* password = "123456789";

// // Set web server port number to 80
WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

void loop() {
}

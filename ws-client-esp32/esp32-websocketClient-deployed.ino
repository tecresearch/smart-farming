#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "TCA@Admin";
const char* password = "Shivam@9211";

// WebSocket server details
const char* websocket_server = "smart-farming-v1.onrender.com";  // Your computer's IP
const uint16_t websocket_port = 443;          // Match the port in your HTML file

WebSocketsClient webSocket;

// Sensor simulation variables
unsigned long lastDataSend = 0;
const long dataInterval = 5000;  // Send data every 5 seconds
unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 3000; // Reconnect every 3 seconds if disconnected

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi with improved reliability
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Configure WebSocket connection with better settings
  webSocket.begin(websocket_server, websocket_port, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(2000);  // Faster reconnection
  webSocket.enableHeartbeat(15000, 3000, 2);  // Ping every 15s, timeout after 3s, 2 retries
}

void loop() {
  webSocket.loop();
  
  // Handle WiFi and WebSocket reconnection
  if (WiFi.status() != WL_CONNECTED) {
    attemptWiFiReconnect();
  } else if (!webSocket.isConnected()) {
    attemptWebSocketReconnect();
  }
  
  // Send sensor data at regular intervals when connected
  if (webSocket.isConnected() && millis() - lastDataSend > dataInterval) {
    lastDataSend = millis();
    sendSensorData();
  }
}

void attemptWiFiReconnect() {
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt > reconnectInterval) {
    lastAttempt = millis();
    Serial.println("Attempting WiFi reconnection...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}

void attemptWebSocketReconnect() {
  if (millis() - lastReconnectAttempt > reconnectInterval) {
    lastReconnectAttempt = millis();
    Serial.println("Attempting WebSocket reconnection...");
    webSocket.begin(websocket_server, websocket_port, "/");
  }
}
float getTemperature(){
    return random(150, 351) / 10.0;  // 15.0-35.0°C 
}

void sendSensorData() {
  // Create a JSON document with increased capacity
  DynamicJsonDocument doc(512);
  
  // Generate random sensor ID (1-100)
  int sensorId = random(1, 10);
  doc["sensorId"] = "ESP32_" + String(sensorId);
  
  // Generate realistic sensor values based on type
  int sensorType = sensorId % 4;  // 0-3 for different sensor types
  
  switch(sensorType) {
    case 0:  // Temperature
      doc["temperature"] =getTemperature();
      break;
    case 1:  // Humidity
      doc["humidity"] = random(300, 860) / 10.0;     // 30.0-85.0% gethumidity();
      break;
    case 2:  // Soil moisture
      doc["soilMoisture"] = random(100, 901) / 10.0; // 10.0-90.0% getsoilMoisture();
      break;
    case 3:  // Light level
      doc["lightLevel"] = random(100, 1001);         // 100-1000 lux getlightLevel();
      break;
  }
  
  // Add timestamp
  doc["timestamp"] = millis();
  
  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send via WebSocket with error handling
  if (webSocket.sendTXT(jsonString)) {
    Serial.println("Sent: " + jsonString);
  } else {
    Serial.println("Failed to send data");
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected to server");
      // Send initial handshake
      webSocket.sendTXT("{\"type\":\"handshake\",\"device\":\"ESP32\"}");
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] Received text: %s\n", payload);
      // Handle server pings
      if (strncmp((char*)payload, "ping", 4) == 0) {
        webSocket.sendTXT("pong");
      }
      break;
    case WStype_ERROR:
      Serial.printf("[WSc] Error: %s\n", payload);
      break;
    case WStype_PING:
      Serial.println("[WSc] Received ping");
      break;
    case WStype_PONG:
      Serial.println("[WSc] Received pong");
      break;
  }
}
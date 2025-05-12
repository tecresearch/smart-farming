#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// WebSocket server details
const char* websocket_server = "production-xhmr.onrender.com";
const uint16_t websocket_port = 443;

WebSocketsClient webSocket;

// Sensor simulation variables
unsigned long lastDataSend = 0;
const long dataInterval = 5000;
unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 3000;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Configure WebSocket
  webSocket.beginSSL(websocket_server, websocket_port, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(2000);
  webSocket.enableHeartbeat(15000, 3000, 2);
 
}

void loop() {
  webSocket.loop();

  // Reconnect WiFi if needed
  if (WiFi.status() != WL_CONNECTED) {
    attemptWiFiReconnect();
  } else if (!webSocket.isConnected()) {
    attemptWebSocketReconnect();
  }

  // Send data
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

float getTemperature() {
  return random(150, 351) / 10.0;  // 15.0–35.0°C
}

void sendSensorData() {
  DynamicJsonDocument doc(512);

  int sensorId = random(1, 10);
  doc["sensorId"] = "ESP32_" + String(sensorId);

  int sensorType = sensorId % 4;
  switch(sensorType) {
    case 0:
      doc["temperature"] = getTemperature();
      break;
    case 1:
      doc["humidity"] = random(300, 860) / 10.0;
      break;
    case 2:
      doc["soilMoisture"] = random(100, 901) / 10.0;
      break;
    case 3:
      doc["lightLevel"] = random(100, 1001);
      break;
  }

  doc["timestamp"] = millis();

  String jsonString;
  serializeJson(doc, jsonString);

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
      webSocket.sendTXT("{\"type\":\"handshake\",\"device\":\"ESP32\"}");
      break;
    case WStype_TEXT: {
      String message = "";
      for (size_t i = 0; i < length; i++) {
        message += (char)payload[i];
      }
      Serial.println("[WSc] Received text: " + message);
      if (message.startsWith("ping")) {
        webSocket.sendTXT("pong");
      }
      break;
    }
    case WStype_ERROR:
      Serial.println("[WSc] Error occurred.");
      break;
    case WStype_PING:
      Serial.println("[WSc] Received ping");
      break;
    case WStype_PONG:
      Serial.println("[WSc] Received pong");
      break;
  }
}
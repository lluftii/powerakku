#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include "RuipuBattery.h"
#include "config.h"

// HardwareSerial: Serial2 für RX/TX z.B. an GPIO16 (RX), GPIO17 (TX)
#define BMS_RX 20
#define BMS_TX 21

HardwareSerial MySerial(2); // Nutzung von UART2
RuipuBattery myBattery(&MySerial);

// MQTT Setup
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long timer = 0;
bool haveReadData = false;

void setup_wifi() {
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WLAN verbunden");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Verbindung zu MQTT...");
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("verbunden");
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println(" Warte 2 Sekunden");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  MySerial.begin(9600, SERIAL_8N1, BMS_RX, BMS_TX); // UART für Ruipu BMS
  delay(100);

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - timer > 1000) {
    timer = millis();
    myBattery.unlock(); // BMS entsperren
    haveReadData = false;
  }

  if (myBattery.read() && !haveReadData) {
    haveReadData = true;

    // Werte als JSON
    String payload = "{";
    payload += "\"soc\":" + String(myBattery.soc()) + ",";
    payload += "\"voltage\":" + String(myBattery.voltage()) + ",";
    payload += "\"current\":" + String(myBattery.current()) + ",";
    payload += "\"low\":" + String(myBattery.low()) + ",";
    payload += "\"high\":" + String(myBattery.high()) + ",";
    payload += "\"maxTemp\":" + String(myBattery.maxTemp()) + ",";
    payload += "\"minTemp\":" + String(myBattery.minTemp());
    payload += "}";

    client.publish(MQTT_TOPIC, payload.c_str(), true);
    Serial.println(payload);
  }
}

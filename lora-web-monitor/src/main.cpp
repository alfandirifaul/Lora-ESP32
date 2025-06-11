#include <Arduino.h>
#include <WiFi.h>
#include "WebServer.h"
#include "WiFiManager.h"
#include "DataLogger.h"
#include "LoRaReceiver.h"

WebServer webServer;
WiFiManager wifiManager;
DataLogger dataLogger;
LoRaReceiver loraReceiver;

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing...");

    wifiManager.connectToWiFi();
    loraReceiver.initialize();
    dataLogger.initialize();
    
    webServer.startServer();
}

void loop() {
    webServer.handleClient();
    loraReceiver.receiveData();
    dataLogger.logData();
}
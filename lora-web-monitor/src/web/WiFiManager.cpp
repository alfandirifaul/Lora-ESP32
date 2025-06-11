#include "WiFiManager.h"
#include <WiFi.h>

WiFiManager::WiFiManager() {}

void WiFiManager::connectToWiFi(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\n✅ Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void WiFiManager::handleWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("🔴 WiFi disconnected, attempting to reconnect...");
        connectToWiFi("your_ssid", "your_password"); // Replace with actual SSID and password
    }
}
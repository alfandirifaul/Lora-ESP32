#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
    void connectToWiFi(const char* ssid, const char* password);
    void handleWiFiConnection();
    void disconnectFromWiFi();
    bool isConnected();
    String getCurrentSSID();
};

#endif // WIFIMANAGER_H
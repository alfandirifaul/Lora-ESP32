#ifndef WIFIHANDLER_H
#define WIFIHANDLER_H

#include <Arduino.h>
#include <WebServer.h>

class WiFiHandler {
public:
    WiFiHandler(WebServer& server);
    void handleWiFiRequest();
    void connectToWiFi(const String& ssid, const String& password);
    void handleWiFiConnection();

private:
    WebServer& server;
};

#endif // WIFIHANDLER_H
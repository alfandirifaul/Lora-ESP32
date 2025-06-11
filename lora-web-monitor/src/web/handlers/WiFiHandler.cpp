#include "WiFiHandler.h"
#include <WiFi.h>

WiFiHandler::WiFiHandler() {}

void WiFiHandler::handleWiFiRequest(WebServer& server) {
    if (server.method() == HTTP_GET) {
        server.send(200, "text/html", getWiFiConfigPage());
    } else if (server.method() == HTTP_POST) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        connectToWiFi(ssid, password);
        server.send(200, "text/html", "<h1>WiFi Configuration Updated</h1><p>Reconnecting...</p>");
    }
}

void WiFiHandler::connectToWiFi(const String& ssid, const String& password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

String WiFiHandler::getWiFiConfigPage() {
    String page = "<html><body><h1>WiFi Configuration</h1>";
    page += "<form method='POST'>";
    page += "SSID: <input type='text' name='ssid'><br>";
    page += "Password: <input type='password' name='password'><br>";
    page += "<input type='submit' value='Connect'>";
    page += "</form></body></html>";
    return page;
}
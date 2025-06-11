#include <WiFi.h>
#include <WebServer.h>
#include "WebServer.h"
#include "WiFiManager.h"
#include "handlers/StatusHandler.h"
#include "handlers/LogHandler.h"
#include "handlers/WiFiHandler.h"

WebServer server(80);
WiFiManager wifiManager;
StatusHandler statusHandler;
LogHandler logHandler;
WiFiHandler wifiHandler;

void WebServer::begin() {
    server.on("/", HTTP_GET, [this]() { statusHandler.handleStatusRequest(); });
    server.on("/logs", HTTP_GET, [this]() { logHandler.handleLogRequest(); });
    server.on("/wifi", HTTP_GET, [this]() { wifiHandler.handleWiFiRequest(); });
    
    server.begin();
}

void WebServer::handleClient() {
    server.handleClient();
}
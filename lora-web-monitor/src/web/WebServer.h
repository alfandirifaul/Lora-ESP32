#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <WiFi.h>
#include <WebServer.h>

class WebServer {
public:
    WebServer();
    void startServer();
    void handleClient();

private:
    WebServer server;
    void setupRoutes();
};

#endif // WEBSERVER_H
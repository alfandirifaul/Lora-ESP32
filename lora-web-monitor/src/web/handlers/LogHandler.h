#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <Arduino.h>
#include <WebServer.h>

class LogHandler {
public:
    LogHandler(WebServer& server);
    void handleLogRequest();

private:
    WebServer& server;
};

#endif // LOGHANDLER_H
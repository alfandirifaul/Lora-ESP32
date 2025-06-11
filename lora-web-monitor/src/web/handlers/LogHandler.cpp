#include "LogHandler.h"
#include <ArduinoJson.h>
#include <DataLogger.h>

LogHandler::LogHandler(DataLogger* logger) : dataLogger(logger) {}

void LogHandler::handleLogRequest(AsyncWebServerRequest* request) {
    String logs = dataLogger->getLogs();
    request->send(200, "application/json", logs);
}
#include "StatusHandler.h"
#include <ArduinoJson.h>
#include "LoRaReceiver.h"
#include "DataLogger.h"

extern LoRaReceiver loraReceiver;
extern DataLogger dataLogger;

void StatusHandler::handleStatusRequest(WebServer& server) {
    // Create a JSON object to hold the status information
    StaticJsonDocument<256> jsonDoc;

    // Populate the JSON object with LoRa status and logs
    jsonDoc["status"] = loraReceiver.getStatus();
    jsonDoc["lastReceivedData"] = loraReceiver.getLastReceivedData();
    jsonDoc["logs"] = dataLogger.getLogs();

    // Serialize the JSON object to a string
    String response;
    serializeJson(jsonDoc, response);

    // Send the response back to the client
    server.send(200, "application/json", response);
}
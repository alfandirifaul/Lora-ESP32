#include "DataLogger.h"

DataLogger::DataLogger() {
    // Initialize log storage
    logIndex = 0;
}

void DataLogger::logData(const String& data) {
    if (logIndex < MAX_LOG_ENTRIES) {
        logs[logIndex++] = data;
    } else {
        // Optionally handle log overflow, e.g., overwrite oldest entry
        for (int i = 1; i < MAX_LOG_ENTRIES; i++) {
            logs[i - 1] = logs[i];
        }
        logs[MAX_LOG_ENTRIES - 1] = data;
    }
}

String DataLogger::getLogs() const {
    String allLogs;
    for (int i = 0; i < logIndex; i++) {
        allLogs += logs[i] + "\n";
    }
    return allLogs;
}
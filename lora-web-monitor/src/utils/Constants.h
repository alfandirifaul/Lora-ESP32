#ifndef CONSTANTS_H
#define CONSTANTS_H

// WiFi connection constants
const char* WIFI_SSID = "Your_SSID";
const char* WIFI_PASSWORD = "Your_PASSWORD";

// LoRa configuration constants
const long LORA_FREQUENCY = 433E6; // LoRa frequency in Hz
const int LORA_SPREADING_FACTOR = 7; // Spreading factor
const int LORA_BANDWIDTH = 125E3; // Bandwidth in Hz
const int LORA_CODING_RATE = 5; // Coding rate

// Web server constants
const int WEB_SERVER_PORT = 80; // Web server port

// Log file settings
const char* LOG_FILE_PATH = "/data/logs.txt"; // Path to log file
const int MAX_LOG_ENTRIES = 100; // Maximum number of log entries

// Other constants
const unsigned long STATUS_UPDATE_INTERVAL = 30000; // Status update interval in milliseconds

#endif // CONSTANTS_H
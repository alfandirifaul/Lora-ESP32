#ifndef CONFIG_H
#define CONFIG_H

const char* WIFI_SSID = "your_wifi_ssid";
const char* WIFI_PASSWORD = "your_wifi_password";

const char* SERVER_HOST = "0.0.0.0";
const int SERVER_PORT = 80;

const char* LORA_FREQUENCY = "433E6";
const int LORA_SPREADING_FACTOR = 7;
const int LORA_BANDWIDTH = 125E3;
const int LORA_CODING_RATE = 5;

const unsigned long STATUS_UPDATE_INTERVAL = 30000; // 30 seconds
const unsigned long LOGGING_INTERVAL = 10000; // 10 seconds

#endif // CONFIG_H
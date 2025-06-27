# ESP32 WiFi Manager (PlatformIO)

This project is a PlatformIO-based ESP32 WiFi Manager using ESPAsyncWebServer, AsyncTCP, and LittleFS. 

## Getting Started
1. Open this folder in VS Code with the PlatformIO extension installed.
2. Build and upload the project to your ESP32 Dev Module.
3. Serial monitor baud rate: 115200.

## Libraries
- ESP Async WebServer
- ESPAsyncTCP
- LittleFS_esp32

## File Structure
- `src/main.cpp`: Main source file (converted from Arduino `.ino`)
- `platformio.ini`: PlatformIO configuration

## Usage
- On first boot, the ESP32 will start as an access point for WiFi configuration.
- After configuration, it will connect to your WiFi and serve the web interface.

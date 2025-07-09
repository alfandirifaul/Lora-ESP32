#define BLYNK_TEMPLATE_ID "TMPL6xc3jF9IV"
#define BLYNK_TEMPLATE_NAME "LoRa"

#include "LittleFS.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LoRa.h>
#include <SPI.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <BlynkSimpleEsp32.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

// Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";

IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 255, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval =
    10000; // interval to wait for Wi-Fi connection (milliseconds)

// WiFi Manager Credentials
const char *wifiManagerSSID = "LoRa-Security-WIFI-MANAGER";
const char *wifiManagerPassword = "LoRa1234"; // Open Access Point

// ========== BLYNK CONFIGURATION ==========
#define BLYNK_PRINT Serial
#define BLYNK_EVENT_MOTION "alert_motion_detection"
#define BLYNK_EVENT_LORA_CONNECTION "connection_lost"

// Blynk Auth Token (replace with your own)
char blynkAuth[] = "C6Glsc-JhP-2p0dSrQXWduvI1HSp6pl1";

// Blynk Virtual Pins
#define VPIN_LED_TRANSMITTER V0
#define VPIN_ALERT_DATA V1
#define VPIN_LED_RECEIVER V2
#define VPIN_BUZZER V4

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Read File from LittleFS
String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Delete configuration files
void deleteConfigFiles() {
  Serial.println("🗑️ Deleting configuration files...");

  if (LittleFS.remove(ssidPath)) {
    Serial.println("✅ Deleted: " + String(ssidPath));
  } else {
    Serial.println("❌ Failed to delete: " + String(ssidPath));
  }

  if (LittleFS.remove(passPath)) {
    Serial.println("✅ Deleted: " + String(passPath));
  } else {
    Serial.println("❌ Failed to delete: " + String(passPath));
  }

  if (LittleFS.remove(ipPath)) {
    Serial.println("✅ Deleted: " + String(ipPath));
  } else {
    Serial.println("❌ Failed to delete: " + String(ipPath));
  }

  if (LittleFS.remove(gatewayPath)) {
    Serial.println("✅ Deleted: " + String(gatewayPath));
  } else {
    Serial.println("❌ Failed to delete: " + String(gatewayPath));
  }

  Serial.println("🔄 Configuration files deleted. Device will restart in WiFi "
                 "Manager mode.");
}

// Forward declarations
void showWiFiConnectingScreen();
void showWiFiFailedScreen();

// Initialize WiFi
bool initWiFi() {
  if (ssid == "" || ip == "") {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  
  // Show connecting screen
  showWiFiConnectingScreen();

  // localIP.fromString(ip.c_str());
  // localGateway.fromString(gateway.c_str());

  // if (!WiFi.config(localIP, localGateway, subnet)){
  //   Serial.println("STA Failed to configure");
  //   return false;
  // }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      showWiFiFailedScreen();
      delay(2000);
      return false;
    }
    delay(100);
  }

  Serial.println("WiFi connected successfully.");
  Serial.println(WiFi.localIP());
  return true;
}

// ========== RECEIVER SYSTEM INCLUDES ==========
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <LoRa.h>
#include <SPI.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

// ========== PIN DEFINITIONS ==========
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2
#define BUZZER_PIN 12
#define RESET_BUTTON_PIN 25 // Physical reset button pin

// ========== DISPLAY CONFIGURATION ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========== DISPLAY FUNCTIONS (IMPLEMENTATIONS) ==========
void showWiFiConnectingScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi");
  display.println("=================");
  display.println("SSID:");
  display.println(ssid);
  display.println("");
  display.println("Please wait...");
  display.display();
}

void showWiFiFailedScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("WiFi Connection");
  display.println("Failed!");
  display.println("===============");
  display.println("Starting AP Mode");
  display.println("");
  display.println("Please configure");
  display.println("WiFi settings");
  display.display();
}

// ========== SYSTEM STATE VARIABLES ==========
struct SystemState {
  int motionCount = 0;
  unsigned long lastMotionTime = 0;
  String lastRSSI = "";
  String lastMessage = "";
  bool alarmActive = false;
  unsigned long alarmStartTime = 0;
  unsigned long lastBeepTime = 0;
  bool buzzerState = false;
  unsigned long systemStartTime = 0;
  int maxRSSI = -999;
  int minRSSI = 999;
  bool displayNeedsUpdate = true;
  int animationFrame = 0;
  unsigned long lastAnimationTime = 0;
  volatile bool packetReceived = false;
  bool isBusy = false;
  bool isReady = true;
  unsigned long lastStatusSent = 0;
  unsigned long statusInterval = 30000;
  String receiverID = "";
  bool prevBusy = false;
  bool prevAlarmActive = false;
  bool prevReady = true;
  bool statusChanged = false;
};
SystemState state;

// ========== RESET BUTTON VARIABLES ==========
volatile bool resetButtonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long buttonPressStart = 0;
const unsigned long debounceDelay = 50;    // 50ms debounce delay
const unsigned long longPressDelay = 2000; // 2 seconds for long press
bool isLongPress = false;
bool buttonCurrentlyPressed = false;

// ========== PROFESSIONAL LOGGING SYSTEM ==========
enum LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_CRITICAL };
class ProfessionalLogger {
private:
  String deviceName;
  unsigned long bootTime;
  int logCounter;

public:
  ProfessionalLogger()
      : deviceName("UNKNOWN"), bootTime(millis()), logCounter(0) {}
  ProfessionalLogger(String name)
      : deviceName(name), bootTime(millis()), logCounter(0) {}
  void init() {
    Serial.println(
        "\n╔══════════════════════════════════════════════════════════════╗");
    Serial.println(
        "║                    PROFESSIONAL LOGGING SYSTEM               ║");
    Serial.println(
        "║                     Motion Detection Receiver                ║");
    Serial.println(
        "╠══════════════════════════════════════════════════════════════╣");
    Serial.printf("║ Device ID: %-49s ║\n", deviceName.c_str());
    Serial.printf("║ Boot Time: %-49s ║\n", getFormattedTime().c_str());
    Serial.printf("║ Version: %-51s ║\n", "v3.0 Professional Edition");
    Serial.println(
        "╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
  }
  void setDeviceName(String name) { deviceName = name; }
  void log(LogLevel level, String category, String message, String data = "") {
    logCounter++;
    String timestamp = getFormattedTime();
    String levelStr = getLevelString(level);
    String levelIcon = getLevelIcon(level);
    Serial.printf("[%s] %s [%s] %s: %s\n", timestamp.c_str(), levelIcon.c_str(),
                  category.c_str(), levelStr.c_str(), message.c_str());
    if (data.length() > 0) {
      Serial.printf("    └─ Data: %s\n", data.c_str());
    }
  }
  void logMotionEvent(int alertNumber, String rssi, String sensorData) {
    String separator =
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    Serial.println(separator);
    Serial.printf("🚨 MOTION ALERT RECEIVED #%d\n", alertNumber);
    Serial.println(separator);
    log(LOG_CRITICAL, "MOTION", "Motion detection alert received", sensorData);
    log(LOG_INFO, "SIGNAL", "Signal strength recorded",
        "RSSI: " + rssi + " dBm");
    Serial.printf(
        "┌─ Alert Details ────────────────────────────────────────────┐\n");
    Serial.printf("│ Alert Number: %-44d │\n", alertNumber);
    Serial.printf("│ Signal Strength: %-39s │\n", (rssi + " dBm").c_str());
    Serial.printf("│ Reception Time: %-40s │\n", getCurrentTime().c_str());
    Serial.printf("│ System Uptime: %-41s │\n",
                  formatUptime((millis() - bootTime) / 1000).c_str());
    Serial.printf(
        "└────────────────────────────────────────────────────────────┘\n");
  }
  void logAlarmSequence(String action, String details = "") {
    LogLevel level = action.indexOf("ACTIVATED") >= 0 ? LOG_CRITICAL : LOG_INFO;
    log(level, "ALARM", "Emergency alarm " + action, details);
    if (action.indexOf("ACTIVATED") >= 0) {
      Serial.printf(
          "┌─ ALARM SEQUENCE ───────────────────────────────────────────┐\n");
      Serial.printf("│ Status: %-51s │\n", "🚨 ACTIVATED");
      Serial.printf("│ Duration: %-47s │\n", "5 seconds");
      Serial.printf("│ Buzzer: %-50s │\n", "Active");
      Serial.printf("│ Notification: %-42s │\n", "Telegram sent");
      Serial.printf(
          "└────────────────────────────────────────────────────────────┘\n");
    }
  }
  void logTelegramEvent(String event, bool success, String details = "") {
    LogLevel level = success ? LOG_INFO : LOG_ERROR;
    String status = success ? "SUCCESS" : "FAILED";
    log(level, "TELEGRAM", event + " - " + status, details);
  }
  void logSystemStats() {
    unsigned long uptime = (millis() - bootTime) / 1000;
    Serial.printf(
        "┌─ System Performance ───────────────────────────────────────┐\n");
    Serial.printf("│ Total Detections: %-37d │\n", state.motionCount);
    Serial.printf("│ System Uptime: %-41s │\n", formatUptime(uptime).c_str());
    Serial.printf("│ Memory Usage: %-42s │\n", getMemoryUsage().c_str());
    Serial.printf("│ WiFi Status: %-43s │\n",
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.printf("│ LoRa Status: %-43s │\n", "Active");
    Serial.printf(
        "└────────────────────────────────────────────────────────────┘\n");
  }
  void logStatusUpdate(String transmitterId, bool busy, bool alarm,
                       bool ready) {
    String statusData = "Busy: " + String(busy ? "YES" : "NO") +
                        ", Alarm: " + String(alarm ? "YES" : "NO") +
                        ", Ready: " + String(ready ? "YES" : "NO");
    log(LOG_INFO, "STATUS", "Status update sent to " + transmitterId,
        statusData);
  }

private:
  String getFormattedTime() {
    unsigned long totalSeconds = (millis() - bootTime) / 1000;
    unsigned long hours = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    char timeStr[20];
    sprintf(timeStr, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(timeStr);
  }
  String getLevelString(LogLevel level) {
    switch (level) {
    case LOG_DEBUG:
      return "DEBUG";
    case LOG_INFO:
      return "INFO ";
    case LOG_WARNING:
      return "WARN ";
    case LOG_ERROR:
      return "ERROR";
    case LOG_CRITICAL:
      return "CRIT ";
    default:
      return "UNKN ";
    }
  }
  String getLevelIcon(LogLevel level) {
    switch (level) {
    case LOG_DEBUG:
      return "🔍";
    case LOG_INFO:
      return "ℹ️ ";
    case LOG_WARNING:
      return "⚠️ ";
    case LOG_ERROR:
      return "❌";
    case LOG_CRITICAL:
      return "🚨";
    default:
      return "❓";
    }
  }
  String formatUptime(unsigned long seconds) {
    if (seconds < 60)
      return String(seconds) + "s";
    if (seconds < 3600)
      return String(seconds / 60) + "m " + String(seconds % 60) + "s";
    return String(seconds / 3600) + "h " + String((seconds % 3600) / 60) + "m";
  }
  String getMemoryUsage() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float usage = ((float)usedHeap / totalHeap) * 100;
    return String(usage, 1) + "% (" + String(usedHeap) + "/" +
           String(totalHeap) + ")";
  }
  String getCurrentTime() {
    unsigned long currentTime = (millis() - bootTime) / 1000;
    int hours = (currentTime / 3600) % 24;
    int minutes = (currentTime / 60) % 60;
    int seconds = currentTime % 60;
    return String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) +
           ":" + (seconds < 10 ? "0" : "") + String(seconds);
  }
};
ProfessionalLogger logger;

// ========== TELEGRAM CONFIGURATION ==========
const String BOT_TOKEN = "7472014617:AAGYjDzTqek4QaTJfDMiiO2yldJsq65HK1w";
const String CHAT_ID = "7651348719";
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ========== UTILITY FUNCTIONS ==========
String getCurrentTime() {
  unsigned long currentTime = (millis() - state.systemStartTime) / 1000;
  int hours = (currentTime / 3600) % 24;
  int minutes = (currentTime / 60) % 60;
  int seconds = currentTime % 60;
  return String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) +
         ":" + (seconds < 10 ? "0" : "") + String(seconds);
}
String formatUptime(unsigned long seconds) {
  if (seconds < 60)
    return String(seconds) + "s";
  if (seconds < 3600)
    return String(seconds / 60) + "m " + String(seconds % 60) + "s";
  return String(seconds / 3600) + "h " + String((seconds % 3600) / 60) + "m";
}

// ========== DISPLAY FUNCTIONS ==========
void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Lora Security System");
  display.println("==================");
  display.println("Initializing...");
  display.println("");
  display.printf("Device: %s", state.receiverID.c_str());
  display.display();
}
void showInitializingScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing LoRa...");
  display.display();
}
void showReadyScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Ready!");
  display.println("==============");
  display.printf("ID: %s\n", state.receiverID.c_str());
  display.printf("WiFi: Connected\n");
  display.printf("LoRa: Active\n");
  display.println("Monitoring...");
  display.display();
}
void showEmergencyScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println("!!ALERT!!");
  display.drawFastHLine(0, 20, display.width(), SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.printf("Motion Detected!");
  display.setCursor(0, 40);
  display.printf("Signal: %s dBm\n", state.lastRSSI.c_str());
  display.setCursor(0, 52);
  display.printf("Time: %s", getCurrentTime().c_str());
  display.display();
}
void showWiFiManagerScreen(const char *ssid, const char *password,
                           IPAddress apIP) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("WiFi Manager Mode");
  display.println("================");
  display.println("SSID:");
  display.println(ssid);
  display.println("PASS:");
  display.println(password);
  display.print("IP: ");
  display.println(apIP);
  display.display();
}

// ========== INTERRUPT SERVICE ROUTINE ==========
void IRAM_ATTR onLoRaReceive(int packetSize) { state.packetReceived = true; }

// ========== RESET BUTTON INTERRUPT ==========
void IRAM_ATTR onResetButtonPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > debounceDelay) {
    bool buttonState = digitalRead(RESET_BUTTON_PIN) == LOW;
    
    if (buttonState && !buttonCurrentlyPressed) {
      // Button just pressed
      buttonCurrentlyPressed = true;
      buttonPressStart = currentTime;
      resetButtonPressed = true;
    } else if (!buttonState && buttonCurrentlyPressed) {
      // Button just released
      buttonCurrentlyPressed = false;
    }
    lastButtonPress = currentTime;
  }
}

// ========== INITIALIZATION FUNCTIONS ==========
void initializeReceiverID() {
  Serial.print("🆔 Generating receiver ID... ");
  WiFi.mode(WIFI_STA);
  String mac = WiFi.macAddress();
  state.receiverID = "RX-" + mac.substring(12);
  state.receiverID.replace(":", "");
  Serial.print("OK (");
  Serial.print(state.receiverID);
  Serial.println(")");
}
void initializeHardware() {
  logger.log(LOG_INFO, "HARDWARE", "Initializing hardware components", "");
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize reset button
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON_PIN), onResetButtonPress,
                  CHANGE);
  Serial.println("✅ Reset button initialized on pin " +
                 String(RESET_BUTTON_PIN));
  Serial.println(
      "💡 Press < 3s: Normal restart | Press 3s+: Delete config files");

  Wire.begin();
  Serial.println("📺 Initializing OLED Display...");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      logger.log(LOG_ERROR, "HARDWARE", "OLED Display not found",
                 "Tried addresses 0x3C and 0x3D");
      Serial.println("❌ OLED Display not found!");
    } else {
      logger.log(LOG_INFO, "HARDWARE", "OLED Display initialized",
                 "Address: 0x3D");
      Serial.println("✅ OLED found at 0x3D");
    }
  } else {
    logger.log(LOG_INFO, "HARDWARE", "OLED Display initialized",
               "Address: 0x3C");
    Serial.println("✅ OLED found at 0x3C");
  }
  showStartupScreen();
}
void initializeBuzzer() {
  logger.log(LOG_INFO, "BUZZER", "Initialize Buzzer");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  logger.log(LOG_INFO, "BUZZER", "Buzzer Initialize Successfully");
}
void initializeTelegram() {
  logger.log(LOG_INFO, "TELEGRAM", "Initializing Telegram bot", "");
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("✅ Telegram Bot Initialized");
}
void initializeLoRa() {
  logger.log(LOG_INFO, "NETWORK", "Initializing LoRa module", "");
  Serial.println("📡 Initializing LoRa Module...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    showInitializingScreen();
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(6);
  LoRa.onReceive(onLoRaReceive);
  LoRa.receive();
  logger.log(LOG_INFO, "NETWORK", "LoRa module ready",
             "Frequency: 433MHz, SF: 7");
  Serial.println("✅ LoRa Module Ready!");
}

// ========== RECEIVER STATUS FUNCTIONS ==========
void sendReceiverStatus() {
  bool hasChanged = (state.isBusy != state.prevBusy) ||
                    (state.alarmActive != state.prevAlarmActive) ||
                    (state.isReady != state.prevReady) || state.statusChanged;
  bool heartbeatTime = (millis() - state.lastStatusSent) > state.statusInterval;
  if (!hasChanged && !heartbeatTime) {
    return;
  }
  String statusMessage = "{";
  statusMessage += "\"type\":\"STATUS\",";
  statusMessage += "\"id\":\"" + state.receiverID + "\",";
  statusMessage += "\"busy\":" + String(state.isBusy ? "true" : "false") + ",";
  statusMessage +=
      "\"alarm\":" + String(state.alarmActive ? "true" : "false") + ",";
  statusMessage +=
      "\"ready\":" + String(state.isReady ? "true" : "false") + ",";
  statusMessage += "\"time\":" + String(millis());
  statusMessage += "}";
  LoRa.beginPacket();
  LoRa.print(statusMessage);
  bool success = LoRa.endPacket();
  if (success) {
    String changeType = hasChanged ? "CHANGE" : "HEARTBEAT";
    Serial.printf("📡 Status %s sent to transmitter:\n", changeType.c_str());
    Serial.printf("   • Busy: %s%s\n", state.isBusy ? "YES" : "NO",
                  (state.isBusy != state.prevBusy) ? " [CHANGED]" : "");
    Serial.printf("   • Alarm: %s%s\n", state.alarmActive ? "YES" : "NO",
                  (state.alarmActive != state.prevAlarmActive) ? " [CHANGED]"
                                                               : "");
    Serial.printf("   • Ready: %s%s\n", state.isReady ? "YES" : "NO",
                  (state.isReady != state.prevReady) ? " [CHANGED]" : "");
    state.prevBusy = state.isBusy;
    state.prevAlarmActive = state.alarmActive;
    state.prevReady = state.isReady;
    state.statusChanged = false;
    state.lastStatusSent = millis();
    // Blynk: LED receiver ON if ready
    Blynk.virtualWrite(VPIN_LED_RECEIVER, state.isReady ? 1 : 0);
  } else {
    Serial.println("❌ Failed to send status update");
  }
  LoRa.receive();
}

// ========== EMERGENCY ALARM SYSTEM ==========
void startEmergencyAlarm() {
  bool wasAlarmActive = state.alarmActive;
  bool wasReady = state.isReady;
  state.alarmActive = true;
  state.isReady = false;
  state.buzzerState = true;
  Serial.println("🚨 EMERGENCY ALARM ACTIVATED!");
  if (state.alarmActive != wasAlarmActive || state.isReady != wasReady) {
    Serial.println("🚨 Alarm state changed - sending status update");
    sendReceiverStatus();
  }
  // Blynk: Turn on buzzer
  Blynk.virtualWrite(VPIN_BUZZER, 1);
  bool stateBuzzer = false;
  for (int count = 0; count < 200; count++) {
    stateBuzzer = !stateBuzzer;
    digitalWrite(BUZZER_PIN, stateBuzzer);
    delay(100);
    if (state.statusChanged) {
      sendReceiverStatus();
    }
  }
  digitalWrite(BUZZER_PIN, LOW);
  state.alarmActive = false;
  state.isReady = true;
  state.buzzerState = false;
  // Blynk: Turn off buzzer
  Blynk.virtualWrite(VPIN_BUZZER, 0);
  Serial.println("✅ Emergency alarm deactivated");
  Serial.println("✅ Alarm deactivated - sending ready status");
  sendReceiverStatus();
  showReadyScreen();
}
void wifiNotConnectedAlarm() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(400);
}

// ========== TELEGRAM FUNCTIONS ==========
void sendTelegramAlert() {
  String timestamp = getCurrentTime();
  String alertMessage =
      "🚨 *MOTION DETECTED!*\n\n"
      "⚠️ *SECURITY ALERT #" +
      String(state.motionCount) +
      "*\n"
      "🕐 *Time:* " +
      timestamp +
      "\n"
      "📍 *Location:* Monitored Area\n"
      "📶 *Signal Strength:* " +
      state.lastRSSI +
      " dBm\n"
      "📨 *Sensor Data:* " +
      state.lastMessage +
      "\n\n"
      "━━━━━━━━━━━━━━━━━━━━━━━━\n"
      "📊 *SYSTEM INFO:*\n"
      "🔋 Uptime: " +
      formatUptime((millis() - state.systemStartTime) / 1000) +
      "\n"
      "🎯 Total Detections: " +
      String(state.motionCount) +
      "\n"
      "📡 LoRa Status: Active\n\n"
      "✅ *Local alarm activated*\n"
      "🔔 _Please check monitored area!_";
  bool success = bot.sendMessage(CHAT_ID, alertMessage, "Markdown");
  logger.logTelegramEvent("Motion alert", success,
                          success ? "Message delivered" : "Delivery failed");
}

// ========== PACKET PROCESSING ==========
void processReceivedPacket() {
  if (!state.packetReceived)
    return;
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  logger.log(LOG_DEBUG, "NETWORK", "Packet received",
             "Length: " + String(message.length()) + " bytes");
  if (message.indexOf("\"type\":\"MOTION\"") > -1) {
    state.motionCount++;
    state.lastMotionTime = millis();
    state.lastRSSI = String(LoRa.packetRssi());
    state.lastMessage = message;
    logger.logMotionEvent(state.motionCount, state.lastRSSI, message);
    showEmergencyScreen();
    logger.logAlarmSequence("ACTIVATED", "Motion detection triggered");
    // Blynk: Set alert data, buzzer, and event
    Blynk.virtualWrite(VPIN_ALERT_DATA, message);
    Blynk.virtualWrite(VPIN_BUZZER, 1);
    Blynk.logEvent(BLYNK_EVENT_MOTION, "Motion detected!");
    startEmergencyAlarm();
    state.isBusy = true;
    state.isReady = false;
    logger.log(LOG_INFO, "TELEGRAM", "Sending motion alert",
               "Alert");
    sendTelegramAlert();
    state.displayNeedsUpdate = true;
    state.isBusy = false;
    state.isReady = true;
    logger.logStatusUpdate("TRANSMITTER", state.isBusy, state.alarmActive,
                           state.isReady);
  }
  state.packetReceived = false;
}

// ========== DISPLAY UPDATE ==========
void updateDisplay() {
  if (!state.displayNeedsUpdate)
    return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.printf("Lora Security System\n");
  display.printf("==================\n");
  display.printf("Alerts: %d\n", state.motionCount);
  display.printf(
      "Uptime: %s\n",
      formatUptime((millis() - state.systemStartTime) / 1000).c_str());
  display.printf("RSSI: %s dBm\n", state.lastRSSI.c_str());
  display.printf("Status: %s\n", state.isReady ? "Ready" : "Busy");
  display.display();
  state.displayNeedsUpdate = false;
}

// ========== RESET BUTTON HANDLER ==========
void handleResetButton() {
  if (resetButtonPressed) {
    unsigned long currentTime = millis();
    unsigned long pressDuration = currentTime - buttonPressStart;
    
    // Check if button is still pressed and long press duration reached
    if (buttonCurrentlyPressed && pressDuration >= longPressDelay && !isLongPress) {
      isLongPress = true;
      
      Serial.println("🔄 Long press detected! Deleting configuration files...");
      logger.log(LOG_INFO, "SYSTEM", "Long press detected - deleting config files", "2+ seconds press");
      
      // Show config reset message on display
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("CONFIG");
      display.println("RESET");
      display.setTextSize(1);
      display.setCursor(0, 40);
      display.println("Deleting files...");
      display.display();
      
      // Triple beep for config reset
      for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
        delay(150);
      }
      
      // Delete configuration files immediately
      deleteConfigFiles();
      
      // Show restart message
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("RESTARTING");
      display.setTextSize(1);
      display.setCursor(0, 30);
      display.println("Config deleted");
      display.println("Please wait...");
      display.display();
      
      delay(1000);
      Serial.println("🚀 Restarting ESP32 after config reset...");
      ESP.restart();
    }
    
    // Handle button release
    if (!buttonCurrentlyPressed) {
      resetButtonPressed = false;
      
      if (!isLongPress && pressDuration < longPressDelay) {
        // Short press - normal restart
        Serial.println("🔄 Short press detected - normal restart");
        logger.log(LOG_INFO, "SYSTEM", "Manual reset button pressed", "Restarting ESP32");
        
        // Show restart message on display
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("RESTARTING");
        display.setTextSize(1);
        display.setCursor(0, 30);
        display.println("Please wait...");
        display.display();
        
        // Brief buzzer notification
        for (int i = 0; i < 3; i++) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(200);
          digitalWrite(BUZZER_PIN, LOW);
          delay(100);
        }
        
        delay(1000);
        Serial.println("🚀 Restarting ESP32...");
        ESP.restart();
      }
      
      isLongPress = false;
    }
  }
}

// Add a global flag to indicate WiFi Manager (AP) mode
bool inWiFiManagerMode = false;

// ========== WIFI CONNECTION MONITORING ==========
void monitorWiFiConnection() {
  static unsigned long lastWiFiCheck = 0;
  static bool wasConnected = false;
  
  // Check WiFi status every 5 seconds
  if (millis() - lastWiFiCheck > 5000) {
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
    
    if (wasConnected && !currentlyConnected && !inWiFiManagerMode) {
      // WiFi connection lost
      Serial.println("⚠️ WiFi connection lost! Switching to WiFi Manager mode...");
      logger.log(LOG_WARNING, "WIFI", "Connection lost - switching to AP mode", "");
      
      // Show WiFi failed screen
      showWiFiFailedScreen();
      delay(2000);
      
      // Start WiFi Manager mode
      inWiFiManagerMode = true;
      
      // Start AP mode
      WiFi.softAP(wifiManagerSSID, wifiManagerPassword, 1, 0, 4);
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      
      // Show WiFi manager screen
      showWiFiManagerScreen(wifiManagerSSID, wifiManagerPassword, IP);
      
      // Notify with buzzer
      for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }
    }
    
    wasConnected = currentlyConnected;
    lastWiFiCheck = millis();
  }
}

// ========== MAIN SETUP AND LOOP ==========
void setup() {
  Serial.begin(115200);

  initLittleFS();
  initializeHardware();
  initializeBuzzer();
  initializeLoRa();

  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile(LittleFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if (initWiFi()) {
    server.begin();
    server.serveStatic("/", LittleFS, "/");
    Serial.println("Connected to WiFi, starting web server...");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serving index.html");
      request->send(LittleFS, "/index.html", "text/html");
    });
    server.on("/wifimanager.html", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                Serial.println("Serving wifimanager.html");
                request->send(LittleFS, "/wifimanager.html", "text/html");
              });

    // Only after WiFi/AP and web server are up, initialize hardware and LoRa
    state.systemStartTime = millis();
    initializeReceiverID();
    logger.setDeviceName(state.receiverID);
    logger.init();
    logger.log(LOG_INFO, "SYSTEM", "Starting receiver initialization", "");

    initializeTelegram();
    showReadyScreen();
    logger.log(LOG_INFO, "SYSTEM", "System ready for monitoring",
               "All components operational");
    logger.logSystemStats();
    inWiFiManagerMode = false;
    Blynk.begin(blynkAuth, ssid.c_str(), pass.c_str());
  } else {
    Serial.println("Setting AP (Access Point)");
    bool apResult = WiFi.softAP(wifiManagerSSID, wifiManagerPassword, 1, 0, 4);
    if (apResult) {
      Serial.println("[DEBUG] WiFi.softAP started successfully.");
    } else {
      Serial.println("[ERROR] WiFi.softAP failed to start!");
    }
    delay(1000);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.print("AP SSID: ");
    Serial.println(wifiManagerSSID);
    Serial.print("AP Password: ");
    Serial.println(wifiManagerPassword);
    showWiFiManagerScreen(wifiManagerSSID, wifiManagerPassword,
                          IP); // Show info on OLED
    inWiFiManagerMode = true;
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    server.serveStatic("/", LittleFS, "/");
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        const AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            writeFile(LittleFS, passPath, pass.c_str());
          }
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
        }
      }
      request->send(200, "text/plain",
                    "Done. ESP will restart, connect to your router and go to "
                    "IP address: " +
                        ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

void loop() {
  // Handle reset button
  handleResetButton();

  // Monitor WiFi connection status
  monitorWiFiConnection();

  // In WiFi Manager mode, show the WiFi config screen continuously
  if (inWiFiManagerMode) {
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 3000) { // Update every 3 seconds
      IPAddress IP = WiFi.softAPIP();
      showWiFiManagerScreen(wifiManagerSSID, wifiManagerPassword, IP);
      lastDisplayUpdate = millis();
    }
  } else {
    // Normal operation mode
    processReceivedPacket();
    sendReceiverStatus();
    updateDisplay();
    Blynk.run();
  }
  
  delay(50);
}
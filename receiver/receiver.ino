/*********
  Clean Lora Security Detection System
  Enhanced Motion Detection Data Logger with Professional Display
  Fixed Emergency Alarm System with Proper Timing
*********/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// ═══════════════════════════════════════════════════════════
//                    PIN DEFINITIONS
// ═══════════════════════════════════════════════════════════
#define LORA_SS     5
#define LORA_RST    14
#define LORA_DIO0   2
#define BUZZER_PIN  12

// ═══════════════════════════════════════════════════════════
//                    DISPLAY CONFIGURATION
// ═══════════════════════════════════════════════════════════
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ═══════════════════════════════════════════════════════════
//                    PROFESSIONAL LOGGING SYSTEM
// ═══════════════════════════════════════════════════════════

enum LogLevel {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_CRITICAL
};

// Forward declarations
class ProfessionalLogger;
extern ProfessionalLogger logger;

// ═══════════════════════════════════════════════════════════
//                    SYSTEM STATE VARIABLES
// ═══════════════════════════════════════════════════════════
struct SystemState {
  // Motion detection
  int motionCount = 0;
  unsigned long lastMotionTime = 0;
  String lastRSSI = "";
  String lastMessage = "";
  
  // Alarm system
  bool alarmActive = false;
  unsigned long alarmStartTime = 0;
  unsigned long lastBeepTime = 0;
  bool buzzerState = false;
  
  // System info
  unsigned long systemStartTime = 0;
  int maxRSSI = -999;
  int minRSSI = 999;
  
  // Display
  bool displayNeedsUpdate = true;
  int animationFrame = 0;
  unsigned long lastAnimationTime = 0;
  
  // LoRa interrupt
  volatile bool packetReceived = false;
  
  // Receiver status
  bool isBusy = false;
  bool isReady = true;
  unsigned long lastStatusSent = 0;
  unsigned long statusInterval = 30000; // Send heartbeat every 30 seconds only
  String receiverID = "";
  
  // Previous state tracking for change detection
  bool prevBusy = false;
  bool prevAlarmActive = false;
  bool prevReady = true;
  bool statusChanged = false;
} state;

// ═══════════════════════════════════════════════════════════
//                    NETWORK CONFIGURATION
// ═══════════════════════════════════════════════════════════
const char* WIFI_SSID = "nesyaaa";
const char* WIFI_PASSWORD = "aaaaaaab";
const String BOT_TOKEN = "7472014617:AAGYjDzTqek4QaTJfDMiiO2yldJsq65HK1w";
const String CHAT_ID = "7651348719";

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

const unsigned long BOT_REQUEST_DELAY = 100;
unsigned long lastBotUpdate = 0;

// ═══════════════════════════════════════════════════════════
//                    PROFESSIONAL LOGGING SYSTEM
// ═══════════════════════════════════════════════════════════

class ProfessionalLogger {
private:
  String deviceName;
  unsigned long bootTime;
  int logCounter;
  
public:
  ProfessionalLogger() : deviceName("UNKNOWN"), bootTime(millis()), logCounter(0) {}
  ProfessionalLogger(String name) : deviceName(name), bootTime(millis()), logCounter(0) {}
  
  void init() {
    Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                    PROFESSIONAL LOGGING SYSTEM               ║");
    Serial.println("║                     Motion Detection Receiver                ║");
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    Serial.printf("║ Device ID: %-49s ║\n", deviceName.c_str());
    Serial.printf("║ Boot Time: %-49s ║\n", getFormattedTime().c_str());
    Serial.printf("║ Version: %-51s ║\n", "v3.0 Professional Edition");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
  }
  
  void setDeviceName(String name) {
    deviceName = name;
  }
  
  void log(LogLevel level, String category, String message, String data = "") {
    logCounter++;
    String timestamp = getFormattedTime();
    String levelStr = getLevelString(level);
    String levelIcon = getLevelIcon(level);
    
    Serial.printf("[%s] %s [%s] %s: %s\n", 
                  timestamp.c_str(), 
                  levelIcon.c_str(), 
                  category.c_str(), 
                  levelStr.c_str(), 
                  message.c_str());
    
    if (data.length() > 0) {
      Serial.printf("    └─ Data: %s\n", data.c_str());
    }
  }
  
  void logMotionEvent(int alertNumber, String rssi, String sensorData) {
    String separator = "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    
    Serial.println(separator);
    Serial.printf("🚨 MOTION ALERT RECEIVED #%d\n", alertNumber);
    Serial.println(separator);
    
    log(LOG_CRITICAL, "MOTION", "Motion detection alert received", sensorData);
    log(LOG_INFO, "SIGNAL", "Signal strength recorded", "RSSI: " + rssi + " dBm");
    
    Serial.printf("┌─ Alert Details ────────────────────────────────────────────┐\n");
    Serial.printf("│ Alert Number: %-44d │\n", alertNumber);
    Serial.printf("│ Signal Strength: %-39s │\n", (rssi + " dBm").c_str());
    Serial.printf("│ Reception Time: %-40s │\n", getCurrentTime().c_str());
    Serial.printf("│ System Uptime: %-41s │\n", formatUptime((millis() - bootTime) / 1000).c_str());
    Serial.printf("└────────────────────────────────────────────────────────────┘\n");
  }
  
  void logAlarmSequence(String action, String details = "") {
    LogLevel level = action.indexOf("ACTIVATED") >= 0 ? LOG_CRITICAL : LOG_INFO;
    log(level, "ALARM", "Emergency alarm " + action, details);
    
    if (action.indexOf("ACTIVATED") >= 0) {
      Serial.printf("┌─ ALARM SEQUENCE ───────────────────────────────────────────┐\n");
      Serial.printf("│ Status: %-51s │\n", "🚨 ACTIVATED");
      Serial.printf("│ Duration: %-47s │\n", "5 seconds");
      Serial.printf("│ Buzzer: %-50s │\n", "Active");
      Serial.printf("│ Notification: %-42s │\n", "Telegram sent");
      Serial.printf("└────────────────────────────────────────────────────────────┘\n");
    }
  }
  
  void logTelegramEvent(String event, bool success, String details = "") {
    LogLevel level = success ? LOG_INFO : LOG_ERROR;
    String status = success ? "SUCCESS" : "FAILED";
    log(level, "TELEGRAM", event + " - " + status, details);
  }
  
  void logSystemStats() {
    unsigned long uptime = (millis() - bootTime) / 1000;
    
    Serial.printf("┌─ System Performance ───────────────────────────────────────┐\n");
    Serial.printf("│ Total Detections: %-37d │\n", state.motionCount);
    Serial.printf("│ System Uptime: %-41s │\n", formatUptime(uptime).c_str());
    Serial.printf("│ Memory Usage: %-42s │\n", getMemoryUsage().c_str());
    Serial.printf("│ WiFi Status: %-43s │\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.printf("│ LoRa Status: %-43s │\n", "Active");
    Serial.printf("└────────────────────────────────────────────────────────────┘\n");
  }
  
  void logStatusUpdate(String transmitterId, bool busy, bool alarm, bool ready) {
    String statusData = "Busy: " + String(busy ? "YES" : "NO") + 
                       ", Alarm: " + String(alarm ? "YES" : "NO") + 
                       ", Ready: " + String(ready ? "YES" : "NO");
    
    log(LOG_INFO, "STATUS", "Status update sent to " + transmitterId, statusData);
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
      case LOG_DEBUG: return "DEBUG";
      case LOG_INFO: return "INFO ";
      case LOG_WARNING: return "WARN ";
      case LOG_ERROR: return "ERROR";
      case LOG_CRITICAL: return "CRIT ";
      default: return "UNKN ";
    }
  }
  
  String getLevelIcon(LogLevel level) {
    switch (level) {
      case LOG_DEBUG: return "🔍";
      case LOG_INFO: return "ℹ️ ";
      case LOG_WARNING: return "⚠️ ";
      case LOG_ERROR: return "❌";
      case LOG_CRITICAL: return "🚨";
      default: return "❓";
    }
  }
  
  String formatUptime(unsigned long seconds) {
    if (seconds < 60) return String(seconds) + "s";
    if (seconds < 3600) return String(seconds/60) + "m " + String(seconds%60) + "s";
    return String(seconds/3600) + "h " + String((seconds%3600)/60) + "m";
  }
  
  String getMemoryUsage() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float usage = ((float)usedHeap / totalHeap) * 100;
    
    return String(usage, 1) + "% (" + String(usedHeap) + "/" + String(totalHeap) + ")";
  }
  
  String getCurrentTime() {
    unsigned long currentTime = (millis() - bootTime) / 1000;
    int hours = (currentTime / 3600) % 24;
    int minutes = (currentTime / 60) % 60;
    int seconds = currentTime % 60;
    
    return String(hours) + ":" + 
           (minutes < 10 ? "0" : "") + String(minutes) + ":" + 
           (seconds < 10 ? "0" : "") + String(seconds);
  }
};

// Global logger instance
ProfessionalLogger logger;

// ═══════════════════════════════════════════════════════════
//                    UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════

String getCurrentTime() {
  unsigned long currentTime = (millis() - state.systemStartTime) / 1000;
  int hours = (currentTime / 3600) % 24;
  int minutes = (currentTime / 60) % 60;
  int seconds = currentTime % 60;
  
  return String(hours) + ":" + 
         (minutes < 10 ? "0" : "") + String(minutes) + ":" + 
         (seconds < 10 ? "0" : "") + String(seconds);
}

String formatUptime(unsigned long seconds) {
  if (seconds < 60) return String(seconds) + "s";
  if (seconds < 3600) return String(seconds/60) + "m " + String(seconds%60) + "s";
  return String(seconds/3600) + "h " + String((seconds%3600)/60) + "m";
}

// ═══════════════════════════════════════════════════════════
//                    DISPLAY FUNCTIONS
// ═══════════════════════════════════════════════════════════

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

  // Title
  display.setCursor(10, 0);
  display.println("!!ALERT!!");

  // Separator
  display.drawFastHLine(0, 20, display.width(), SSD1306_WHITE);

  // Details
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.printf("Motion Detected! #%d\n", state.motionCount);
  display.setCursor(0, 40);
  display.printf("Signal: %s dBm\n", state.lastRSSI.c_str());
  display.setCursor(0, 52);
  display.printf("Time: %s", getCurrentTime().c_str());

  display.display();
} 

// ═══════════════════════════════════════════════════════════
//                    INTERRUPT SERVICE ROUTINE
// ═══════════════════════════════════════════════════════════
void IRAM_ATTR onLoRaReceive(int packetSize) {
  state.packetReceived = true;
}

// ═══════════════════════════════════════════════════════════
//                    INITIALIZATION FUNCTIONS
// ═══════════════════════════════════════════════════════════

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
  
  Wire.begin();
  Serial.println("📺 Initializing OLED Display...");
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      logger.log(LOG_ERROR, "HARDWARE", "OLED Display not found", "Tried addresses 0x3C and 0x3D");
      Serial.println("❌ OLED Display not found!");
    } else {
      logger.log(LOG_INFO, "HARDWARE", "OLED Display initialized", "Address: 0x3D");
      Serial.println("✅ OLED found at 0x3D");
    }
  } else {
    logger.log(LOG_INFO, "HARDWARE", "OLED Display initialized", "Address: 0x3C");
    Serial.println("✅ OLED found at 0x3C");
  }
  
  showStartupScreen();
}

void initializeWiFi() {
  logger.log(LOG_INFO, "NETWORK", "Connecting to WiFi", "SSID: " + String(WIFI_SSID));
  Serial.println("🌐 Connecting to WiFi...");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  logger.log(LOG_INFO, "NETWORK", "WiFi connected successfully", "IP: " + WiFi.localIP().toString());
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
  
  logger.log(LOG_INFO, "NETWORK", "LoRa module ready", "Frequency: 433MHz, SF: 7");
  Serial.println("✅ LoRa Module Ready!");
}

// ═══════════════════════════════════════════════════════════
//                    MAIN SETUP FUNCTION
// ═══════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("🚀 Lora Security Detection System Starting...");
  Serial.println("==========================================");
  
  state.systemStartTime = millis();
  
  initializeReceiverID();
  logger.setDeviceName(state.receiverID);
  logger.init();
  
  logger.log(LOG_INFO, "SYSTEM", "Starting receiver initialization", "");
  
  initializeHardware();
  initializeWiFi();
  initializeTelegram();
  initializeLoRa();
  
  showReadyScreen();
  logger.log(LOG_INFO, "SYSTEM", "System ready for monitoring", "All components operational");
  logger.logSystemStats();
}

// ═══════════════════════════════════════════════════════════
//                    MAIN LOOP FUNCTION
// ═══════════════════════════════════════════════════════════

void loop() {
  processReceivedPacket();
  sendReceiverStatus();
  updateDisplay();
  delay(50);
}

// ═══════════════════════════════════════════════════════════
//                    PACKET PROCESSING
// ═══════════════════════════════════════════════════════════

void processReceivedPacket() {
  if (!state.packetReceived) return;
  
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  
  logger.log(LOG_DEBUG, "NETWORK", "Packet received", "Length: " + String(message.length()) + " bytes");
  
  if (message.indexOf("\"type\":\"MOTION\"") > -1) {
    state.motionCount++;
    state.lastMotionTime = millis();
    state.lastRSSI = String(LoRa.packetRssi());
    state.lastMessage = message;

    logger.logMotionEvent(state.motionCount, state.lastRSSI, message);
    showEmergencyScreen();

    logger.logAlarmSequence("ACTIVATED", "Motion detection triggered");
    startEmergencyAlarm();
    
    state.isBusy = true;
    state.isReady = false;

    logger.log(LOG_INFO, "TELEGRAM", "Sending motion alert", "Alert #" + String(state.motionCount));
    sendTelegramAlert();
    
    state.displayNeedsUpdate = true;
    
    state.isBusy = false;
    state.isReady = true;
    
    logger.logStatusUpdate("TRANSMITTER", state.isBusy, state.alarmActive, state.isReady);
  }
  
  state.packetReceived = false;
}

// ═══════════════════════════════════════════════════════════
//                    RECEIVER STATUS FUNCTIONS
// ═══════════════════════════════════════════════════════════

void sendReceiverStatus() {
  bool hasChanged = (state.isBusy != state.prevBusy) ||
                    (state.alarmActive != state.prevAlarmActive) ||
                    (state.isReady != state.prevReady) ||
                    state.statusChanged;
  
  bool heartbeatTime = (millis() - state.lastStatusSent) > state.statusInterval;
  
  if (!hasChanged && !heartbeatTime) {
    return;
  }
  
  String statusMessage = "{";
  statusMessage += "\"type\":\"STATUS\",";
  statusMessage += "\"id\":\"" + state.receiverID + "\",";
  statusMessage += "\"busy\":" + String(state.isBusy ? "true" : "false") + ",";
  statusMessage += "\"alarm\":" + String(state.alarmActive ? "true" : "false") + ",";
  statusMessage += "\"ready\":" + String(state.isReady ? "true" : "false") + ",";
  statusMessage += "\"time\":" + String(millis());
  statusMessage += "}";
  
  LoRa.beginPacket();
  LoRa.print(statusMessage);
  bool success = LoRa.endPacket();
  
  if (success) {
    String changeType = hasChanged ? "CHANGE" : "HEARTBEAT";
    Serial.printf("📡 Status %s sent to transmitter:\n", changeType.c_str());
    Serial.printf("   • Busy: %s%s\n", 
                  state.isBusy ? "YES" : "NO",
                  (state.isBusy != state.prevBusy) ? " [CHANGED]" : "");
    Serial.printf("   • Alarm: %s%s\n", 
                  state.alarmActive ? "YES" : "NO",
                  (state.alarmActive != state.prevAlarmActive) ? " [CHANGED]" : "");
    Serial.printf("   • Ready: %s%s\n", 
                  state.isReady ? "YES" : "NO",
                  (state.isReady != state.prevReady) ? " [CHANGED]" : "");
    
    state.prevBusy = state.isBusy;
    state.prevAlarmActive = state.alarmActive;
    state.prevReady = state.isReady;
    state.statusChanged = false;
    
    state.lastStatusSent = millis();
  } else {
    Serial.println("❌ Failed to send status update");
  }
  
  LoRa.receive();
}

// ═══════════════════════════════════════════════════════════
//                    EMERGENCY ALARM SYSTEM
// ═══════════════════════════════════════════════════════════

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
  
  Serial.println("✅ Emergency alarm deactivated");
  Serial.println("✅ Alarm deactivated - sending ready status");
  sendReceiverStatus();
  showReadyScreen();
}

// ═══════════════════════════════════════════════════════════
//                    TELEGRAM FUNCTIONS
// ═══════════════════════════════════════════════════════════

void sendTelegramAlert() {
  String timestamp = getCurrentTime();
  String alertMessage = 
    "🚨 *MOTION DETECTED!*\n\n"
    "⚠️ *SECURITY ALERT #" + String(state.motionCount) + "*\n"
    "🕐 *Time:* " + timestamp + "\n"
    "📍 *Location:* Monitored Area\n"
    "📶 *Signal Strength:* " + state.lastRSSI + " dBm\n"
    "📨 *Sensor Data:* " + state.lastMessage + "\n\n"
    "━━━━━━━━━━━━━━━━━━━━━━━━\n"
    "📊 *SYSTEM INFO:*\n"
    "🔋 Uptime: " + formatUptime((millis() - state.systemStartTime)/1000) + "\n"
    "🎯 Total Detections: " + String(state.motionCount) + "\n"
    "📡 LoRa Status: Active\n\n"
    "✅ *Local alarm activated*\n"
    "🔔 _Please check monitored area!_";

  bool success = bot.sendMessage(CHAT_ID, alertMessage, "Markdown");
  
  logger.logTelegramEvent("Motion alert", success, success ? "Message delivered" : "Delivery failed");
}

// ═══════════════════════════════════════════════════════════
//                    DISPLAY UPDATE
// ═══════════════════════════════════════════════════════════

void updateDisplay() {
  if (!state.displayNeedsUpdate) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.printf("Lora Security System\n");
  display.printf("==================\n");
  display.printf("Alerts: %d\n", state.motionCount);
  display.printf("Uptime: %s\n", formatUptime((millis() - state.systemStartTime)/1000).c_str());
  display.printf("RSSI: %s dBm\n", state.lastRSSI.c_str());
  display.printf("Status: %s\n", state.isReady ? "Ready" : "Busy");
  
  display.display();
  state.displayNeedsUpdate = false;
}
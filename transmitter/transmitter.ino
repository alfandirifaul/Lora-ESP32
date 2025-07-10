/*********
  Alfandi Nurhuda - PAMS Security System
  Professional Motion Detection Transmitter v3.0
  Enhanced PIR Motion Sensor with Advanced Logging
*********/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <RTClib.h>    // DS3231 RTC Library 
#include <EEPROM.h>

// ═══════════════════════════════════════════════════════════
//                    HARDWARE CONFIGURATION
// ═══════════════════════════════════════════════════════════

// LoRa Module Pin Configuration
#define ss 5
#define rst 14
#define dio0 2

// RTC Module Configuration
// DS3231 uses I2C: SDA (GPIO21), SCL (GPIO22) on ESP32
// #define ALERT_CLOCK_TIME 16 // Send the alert motion after 16 o'clock default time (REMOVED, use variable instead)

// Sensor & Indicator Configuration
#define PIR_PIN 4         // PIR Motion Sensor GPIO
#define BUZZER_PIN 27        // BUZZER GPIO
#define STATUS_LED 12      //  Status LED GPIO
#define RESET_BUTTON_PIN 25 // Physical reset button GPIO


// ═══════════════════════════════════════════════════════════
//                    SYSTEM PARAMETERS
// ═══════════════════════════════════════════════════════════

// Motion Detection Settings
const unsigned long MOTION_DELAY = 3000;        // Cooldown period (3 seconds)
const unsigned long SENSOR_WARMUP = 15000;      // Sensor stabilization time (15 seconds)

// LoRa Configuration
const long FREQUENCY = 433E6;                   // 433MHz
const int TX_POWER = 20;                        // Transmission power
const byte SYNC_WORD = 0xF3;                    // Network sync word

// Queue Configuration
const int MAX_QUEUE_SIZE = 10;                  // Maximum queued messages
const unsigned long RETRY_INTERVAL = 5000;     // Retry transmission every 5 seconds
const int MAX_RETRY_ATTEMPTS = 3;               // Maximum retry attempts per message

// EEPROM Configuration
#define EEPROM_SIZE 4
#define EEPROM_ADDR_ALERT_TIME 0

// ═══════════════════════════════════════════════════════════
//                    QUEUE STRUCTURE
// ═══════════════════════════════════════════════════════════

struct QueuedMessage {
  String type;
  String deviceId;
  int count;
  unsigned long timestamp;
  String datetime;
  String unixTimestamp;
  unsigned long uptime;
  bool rtcAvailable;
  int alertTime;
  int retryCount;
  bool isValid;
};

// ═══════════════════════════════════════════════════════════
//                    GLOBAL VARIABLES
// ═══════════════════════════════════════════════════════════

// Motion Detection Variables
int lastMotionState = LOW;
unsigned long lastMotionTime = 0;
bool motionCooldown = false;
bool systemReady = false;

// Statistics & Logging
unsigned long systemStartTime = 0;
int motionCounter = 0;
int packetsSent = 0;
int transmissionErrors = 0;
String deviceID = "";

// Queue Management
QueuedMessage messageQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueSize = 0;
unsigned long lastRetryTime = 0;
int queuedMessages = 0;
int retriedMessages = 0;

// Alert time configuration
int ALERT_CLOCK_TIME = 16; // Default value, will be loaded from EEPROM

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

struct LogEntry {
  unsigned long timestamp;
  LogLevel level;
  String category;
  String message;
  String data;
};

class ProfessionalLogger {
private:
  String deviceName;
  unsigned long bootTime;
  int logCounter;
  
public:
  ProfessionalLogger(String name) : deviceName(name), bootTime(millis()), logCounter(0) {}
  
  void init() {
    Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                    PROFESSIONAL LOGGING SYSTEM               ║");
    Serial.println("║                    Motion Detection Transmitter              ║");
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    Serial.printf("║ Device ID: %-49s ║\n", deviceName.c_str());
    Serial.printf("║ Boot Time: %-49s ║\n", getFormattedTime().c_str());
    Serial.printf("║ Version: %-51s ║\n", "v3.0 Professional Edition");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
  }
  
  void log(LogLevel level, String category, String message, String data = "") {
    logCounter++;
    String timestamp = getFormattedTime();
    String levelStr = getLevelString(level);
    String levelIcon = getLevelIcon(level);
    
    // Main log line
    Serial.printf("[%s] %s [%s] %s: %s\n", 
                  timestamp.c_str(), 
                  levelIcon.c_str(), 
                  category.c_str(), 
                  levelStr.c_str(), 
                  message.c_str());
    
    // Data payload if provided
    if (data.length() > 0) {
      Serial.printf("    └─ Data: %s\n", data.c_str());
    }
  }
  
  void logMotionEvent(int alertNumber, bool success, String details) {
    String separator = "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    
    Serial.println(separator);
    Serial.printf("🚨 MOTION DETECTION EVENT #%d\n", alertNumber);
    Serial.println(separator);
    
    log(LOG_CRITICAL, "MOTION", "Motion detected and processed", details);
    log(LOG_INFO, "TRANSMISSION", success ? "Alert sent successfully" : "Alert queued for retry", "");
    
    // Event summary
    Serial.printf("┌─ Event Summary ────────────────────────────────────────────┐\n");
    Serial.printf("│ Alert Number: %-44d │\n", alertNumber);
    Serial.printf("│ Status: %-51s │\n", success ? "✅ TRANSMITTED" : "📋 QUEUED");
    Serial.printf("│ Device: %-51s │\n", deviceName.c_str());
    Serial.printf("│ Uptime: %-51s │\n", formatUptime((millis() - bootTime) / 1000).c_str());
    Serial.printf("└────────────────────────────────────────────────────────────┘\n");
    
    logSystemStats();
    Serial.println(separator);
  }
  
  void logSystemStats() {
    Serial.printf("┌─ System Statistics ────────────────────────────────────────┐\n");
    Serial.printf("│ Packets Sent: %-44d │\n", packetsSent);
    Serial.printf("│ Transmission Errors: %-35d │\n", transmissionErrors);
    Serial.printf("│ Queue Size: %-47d │\n", queueSize);
    Serial.printf("│ Queued Messages: %-40d │\n", queuedMessages);
    Serial.printf("│ Retry Attempts: %-41d │\n", retriedMessages);
    Serial.printf("│ Success Rate: %-43.1f%% │\n", getSuccessRate());
    Serial.printf("└────────────────────────────────────────────────────────────┘\n");
  }
  
  void logQueueOperation(String operation, int queueSize, String details = "") {
    String icon = operation.indexOf("enqueue") >= 0 ? "📥" : "📤";
    log(LOG_INFO, "QUEUE", icon + " " + operation, 
        "Queue size: " + String(queueSize) + (details.length() > 0 ? " | " + details : ""));
  }
  
  void logSystemStatus(String component, String status, String details = "") {
    LogLevel level = status.indexOf("OK") >= 0 ? LOG_INFO : 
                    status.indexOf("ERROR") >= 0 ? LOG_ERROR : LOG_WARNING;
    log(level, "SYSTEM", component + ": " + status, details);
  }
  
  void logNetworkEvent(String event, String details) {
    log(LOG_INFO, "NETWORK", event, details);
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
  
  float getSuccessRate() {
    int totalAttempts = packetsSent + transmissionErrors;
    return totalAttempts > 0 ? ((float)packetsSent / totalAttempts) * 100.0 : 0.0;
  }
  
  String formatUptime(unsigned long seconds) {
    if (seconds < 60) return String(seconds) + "s";
    if (seconds < 3600) return String(seconds/60) + "m " + String(seconds%60) + "s";
    return String(seconds/3600) + "h " + String((seconds%3600)/60) + "m";
  }
};

// Global logger instance
ProfessionalLogger logger("UNKNOWN");

volatile bool resetRequested = false;

void IRAM_ATTR handleResetInterrupt() {
  resetRequested = true;
}

// RTC instance
RTC_DS3231 rtc;
bool rtcAvailable = false;

// ═══════════════════════════════════════════════════════════
//                    ENHANCED INITIALIZATION
// ═══════════════════════════════════════════════════════════

void initializeSystem() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  systemStartTime = millis();
  EEPROM.begin(EEPROM_SIZE);
  ALERT_CLOCK_TIME = loadAlertClockTimeFromEEPROM();
  
  // Initialize logger with device ID first
  initializeDeviceID();
  logger = ProfessionalLogger(deviceID);
  logger.init();
  
  logger.log(LOG_INFO, "SYSTEM", "Starting system initialization", "");
  
  initializeGPIO();
  initializeRTC();
  initializeLoRa();
  initializeQueue();
  warmupPIRSensor();
  
  systemReady = true;
  displaySystemReady();
  
  logger.log(LOG_INFO, "SYSTEM", "System initialization completed", "All components operational");
  logger.logSystemStats();
}

void initializeGPIO() {
  logger.log(LOG_INFO, "HARDWARE", "Initializing GPIO pins", "");
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON_PIN), handleResetInterrupt, FALLING);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
  logger.logSystemStatus("GPIO", "OK", "PIR: Pin " + String(PIR_PIN) + ", Buzzer: Pin " + String(BUZZER_PIN));
}

void initializeLoRa() {
  logger.log(LOG_INFO, "NETWORK", "Initializing LoRa module", "");
  
  LoRa.setPins(ss, rst, dio0);
  
  int attempts = 0;
  while (!LoRa.begin(FREQUENCY) && attempts < 10) {
    attempts++;
    delay(500);
    logger.log(LOG_WARNING, "NETWORK", "LoRa initialization attempt " + String(attempts), "");
    
    // Visual feedback during initialization attempts
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
  
  if (attempts >= 10) {
    logger.log(LOG_CRITICAL, "NETWORK", "LoRa initialization FAILED", "Max attempts exceeded");
    blinkError(); // This will loop forever now
  }
  
  // Configure LoRa parameters for optimal performance
  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setTxPower(TX_POWER);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.enableCrc();
  
  String config = "Freq: " + String(FREQUENCY/1E6) + "MHz, Power: " + String(TX_POWER) + "dBm, SF: 7";
  logger.logSystemStatus("LoRa", "OK", config);
}

void initializeRTC() {
  logger.log(LOG_INFO, "HARDWARE", "Initializing DS3231 RTC module", "");
  
  if (!rtc.begin()) {
    logger.log(LOG_WARNING, "HARDWARE", "DS3231 RTC not found - using fallback time", "Check I2C connections (SDA=21, SCL=22)");
    logger.logSystemStatus("RTC", "FALLBACK", "Alert time filtering disabled without RTC");
    rtcAvailable = false;
    return;
  }
  
  rtcAvailable = true;
  
  // Check if RTC lost power and if so, set the time
  if (rtc.lostPower()) {
    logger.log(LOG_WARNING, "HARDWARE", "RTC lost power, setting time", "");
    // Set to compile time if power was lost
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    logger.logSystemStatus("RTC", "TIME SET", "Using compile time");
  }
  
  // Verify RTC is running
  DateTime now = rtc.now();
  if (now.year() < 2020) {
    logger.log(LOG_WARNING, "HARDWARE", "RTC time appears invalid", "Year: " + String(now.year()));
    // Set to compile time if time seems invalid
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    logger.logSystemStatus("RTC", "TIME CORRECTED", "Reset to compile time");
  }
  
  String timeStr = formatDateTime(rtc.now());
  logger.logSystemStatus("RTC", "OK", "Current time: " + timeStr + " | Alert time: " + String(ALERT_CLOCK_TIME) + ":00");
}

void initializeDeviceID() {
  logger.log(LOG_INFO, "SYSTEM", "Generating device ID", "");
  
  WiFi.mode(WIFI_STA);
  String mac = WiFi.macAddress();
  deviceID = "TX-" + mac.substring(12);
  deviceID.replace(":", "");
  
  logger.logSystemStatus("Device ID", "GENERATED", "ID: " + deviceID);
}

// ═══════════════════════════════════════════════════════════
//                    ENHANCED MOTION DETECTION
// ═══════════════════════════════════════════════════════════

void handleMotionDetection() {
  motionCounter++;
  lastMotionTime = millis();
  motionCooldown = true;

  String motionDetails = "Sensor: PIR Pin " + String(PIR_PIN) + ", Time: " + getCurrentDateTime();
  
  // Check if we should send alert based on time
  bool shouldSendAlert = isWithinAlertTime();
  bool success = false;
  
  if (shouldSendAlert) {
    // Try immediate transmission
    success = sendMotionAlert();
    
    // If transmission fails, queue the message
    if (!success) {
      String currentDateTime = getCurrentDateTime();
      String unixTimestamp = getUnixTimestamp();
      unsigned long uptime = (millis() - systemStartTime) / 1000;
      
      enqueueMessage("MOTION", deviceID, motionCounter, millis(), currentDateTime, unixTimestamp, uptime, rtcAvailable, ALERT_CLOCK_TIME);
      logger.logQueueOperation("Message enqueued for retry", queueSize, "Motion alert #" + String(motionCounter));
    }
  } else {
    // Motion detected but outside alert time window
    logger.log(LOG_INFO, "MOTION", "Motion detected outside alert time", 
               "Current time: " + getCurrentDateTime() + " | Alert starts at: " + String(ALERT_CLOCK_TIME) + ":00");
  }
  
  // Visual indication (always show, regardless of time)
  emergencyBuzzer();
  
  // Feed watchdog to prevent reset
  yield();
  
  // Log the detection using professional logger
  logger.logMotionEvent(motionCounter, success, motionDetails + " | Alert sent: " + (shouldSendAlert ? "YES" : "NO"));
}

void emergencyBuzzer() {
  digitalWrite(STATUS_LED, HIGH);
  
  // Non-blocking buzzer pattern - shorter duration to avoid watchdog
  for(int i = 0; i < 10; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);  // Short delay
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);  // Short delay
  }
  
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
}

bool sendMotionAlert() {
  Serial.println("📤 Transmitting motion alert...");
  
  String currentDateTime = getCurrentDateTime();
  String unixTimestamp = getUnixTimestamp();
  unsigned long uptime = (millis() - systemStartTime) / 1000;
  
  LoRa.beginPacket();
  LoRa.print("{");
  LoRa.print("\"type\":\"MOTION\",");
  LoRa.print("\"id\":\"");
  LoRa.print(deviceID);
  LoRa.print("\",");
  LoRa.print("\"count\":");
  LoRa.print(motionCounter);
  LoRa.print(",");
  LoRa.print("\"time\":");
  LoRa.print(millis());
  LoRa.print(",");
  LoRa.print("\"datetime\":\"");
  LoRa.print(currentDateTime);
  LoRa.print("\",");
  LoRa.print("\"timestamp\":");
  LoRa.print(unixTimestamp);
  LoRa.print(",");
  LoRa.print("\"uptime\":");
  LoRa.print(uptime);
  LoRa.print(",");
  LoRa.print("\"rtc_available\":");
  LoRa.print(rtcAvailable ? "true" : "false");
  LoRa.print(",");
  LoRa.print("\"alert_time\":");
  LoRa.print(ALERT_CLOCK_TIME);
  LoRa.print(",");
  LoRa.print("\"queued\":false");
  LoRa.print("}");
  
  bool success = LoRa.endPacket();
  
  if (success) {
    packetsSent++;
  } else {
    transmissionErrors++;
  }
  
  return success;
}

String getCurrentDateTime() {
  if (rtcAvailable) {
    DateTime now = rtc.now();
    return formatDateTime(now);
  } else {
    // Fallback to uptime-based timestamp
    return getFallbackDateTime();
  }
}

String formatDateTime(DateTime dt) {
  char dateTimeStr[20];
  sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
          dt.year(), dt.month(), dt.day(),
          dt.hour(), dt.minute(), dt.second());
  return String(dateTimeStr);
}

String getFallbackDateTime() {
  // Simple uptime-based timestamp when RTC is not available
  unsigned long totalSeconds = (millis() - systemStartTime) / 1000;
  unsigned long hours = (totalSeconds / 3600) % 24;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long seconds = totalSeconds % 60;
  
  char fallbackStr[20];
  sprintf(fallbackStr, "T+%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(fallbackStr);
}

String getUnixTimestamp() {
  if (rtcAvailable) {
    DateTime now = rtc.now();
    return String(now.unixtime());
  } else {
    // Fallback to millis-based timestamp
    return String(millis());
  }
}

bool isWithinAlertTime() {
  if (!rtcAvailable) {
    // If RTC not available, always allow alerts (fallback behavior)
    logger.log(LOG_WARNING, "TIME", "RTC not available - allowing all alerts", "");
    return true;
  }
  
  DateTime now = rtc.now();
  int currentHour = now.hour();
  
  // Send alerts only after ALERT_CLOCK_TIME (default 16:00 / 4 PM)
  bool withinTime = (currentHour >= ALERT_CLOCK_TIME);
  
  if (!withinTime) {
    logger.log(LOG_INFO, "TIME", "Motion detected outside alert window", 
               "Current: " + String(currentHour) + ":00 | Alert starts: " + String(ALERT_CLOCK_TIME) + ":00");
  }
  
  return withinTime;
}

// ═══════════════════════════════════════════════════════════
//                    QUEUE MANAGEMENT FUNCTIONS
// ═══════════════════════════════════════════════════════════

void initializeQueue() {
  logger.log(LOG_INFO, "QUEUE", "Initializing message queue", "Size: " + String(MAX_QUEUE_SIZE));
  
  // Initialize all queue entries as invalid
  for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
    messageQueue[i].isValid = false;
    messageQueue[i].retryCount = 0;
  }
  queueHead = 0;
  queueTail = 0;
  queueSize = 0;
  
  logger.logSystemStatus("Message Queue", "OK", "Capacity: " + String(MAX_QUEUE_SIZE) + " messages");
}

bool enqueueMessage(String type, String deviceId, int count, unsigned long timestamp, String datetime, String unixTimestamp, unsigned long uptime, bool rtcStatus, int alertTime) {
  if (queueSize >= MAX_QUEUE_SIZE) {
    logger.log(LOG_WARNING, "QUEUE", "Queue full, dropping oldest message", "Size: " + String(queueSize));
    dequeueMessage();
  }
  
  messageQueue[queueTail].type = type;
  messageQueue[queueTail].deviceId = deviceId;
  messageQueue[queueTail].count = count;
  messageQueue[queueTail].timestamp = timestamp;
  messageQueue[queueTail].datetime = datetime;
  messageQueue[queueTail].unixTimestamp = unixTimestamp;
  messageQueue[queueTail].uptime = uptime;
  messageQueue[queueTail].rtcAvailable = rtcStatus;
  messageQueue[queueTail].alertTime = alertTime;
  messageQueue[queueTail].retryCount = 0;
  messageQueue[queueTail].isValid = true;
  
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueSize++;
  queuedMessages++;
  
  logger.logQueueOperation("Message enqueued", queueSize, "Type: " + type + ", Count: " + String(count));
  return true;
}

bool dequeueMessage() {
  if (queueSize == 0) return false;
  
  messageQueue[queueHead].isValid = false;
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  queueSize--;
  
  return true;
}

void processMessageQueue() {
  if (queueSize == 0) return;
  if (millis() - lastRetryTime < RETRY_INTERVAL) return;
  
  if (messageQueue[queueHead].isValid) {
    logger.log(LOG_INFO, "QUEUE", "Processing queued message", "Retry: " + String(messageQueue[queueHead].retryCount));
    
    bool success = transmitQueuedMessage(queueHead);
    
    if (success) {
      logger.logQueueOperation("Message sent successfully", queueSize - 1, "Alert #" + String(messageQueue[queueHead].count));
      dequeueMessage();
    } else {
      messageQueue[queueHead].retryCount++;
      retriedMessages++;
      
      if (messageQueue[queueHead].retryCount >= MAX_RETRY_ATTEMPTS) {
        logger.log(LOG_ERROR, "QUEUE", "Message dropped after max retries", 
                  "Alert #" + String(messageQueue[queueHead].count) + ", Attempts: " + String(MAX_RETRY_ATTEMPTS));
        dequeueMessage();
        transmissionErrors++;
      } else {
        logger.log(LOG_WARNING, "QUEUE", "Retry failed, will attempt again", 
                  "Attempt " + String(messageQueue[queueHead].retryCount) + "/" + String(MAX_RETRY_ATTEMPTS));
      }
    }
    
    lastRetryTime = millis();
  }
}

bool transmitQueuedMessage(int index) {
  QueuedMessage& msg = messageQueue[index];
  
  Serial.println("📤 Transmitting queued message...");
  
  LoRa.beginPacket();
  LoRa.print("{");
  LoRa.print("\"type\":\"");
  LoRa.print(msg.type);
  LoRa.print("\",");
  LoRa.print("\"id\":\"");
  LoRa.print(msg.deviceId);
  LoRa.print("\",");
  LoRa.print("\"count\":");
  LoRa.print(msg.count);
  LoRa.print(",");
  LoRa.print("\"time\":");
  LoRa.print(msg.timestamp);
  LoRa.print(",");
  LoRa.print("\"datetime\":\"");
  LoRa.print(msg.datetime);
  LoRa.print("\",");
  LoRa.print("\"timestamp\":");
  LoRa.print(msg.unixTimestamp);
  LoRa.print(",");
  LoRa.print("\"uptime\":");
  LoRa.print(msg.uptime);
  LoRa.print(",");
  LoRa.print("\"rtc_available\":");
  LoRa.print(msg.rtcAvailable ? "true" : "false");
  LoRa.print(",");
  LoRa.print("\"alert_time\":");
  LoRa.print(msg.alertTime);
  LoRa.print(",");
  LoRa.print("\"queued\":true,");
  LoRa.print("\"retry\":");
  LoRa.print(msg.retryCount);
  LoRa.print("}");
  
  bool success = LoRa.endPacket();
  
  if (success) {
    packetsSent++;
  }
  
  return success;
}

// ═══════════════════════════════════════════════════════════
//                    SYSTEM MONITORING
// ═══════════════════════════════════════════════════════════

void updateSystemStatus() {
  static unsigned long lastStatusUpdate = 0;
  static unsigned long lastHeartbeat = 0;
  
  // Heartbeat LED (blinks every 2 seconds when system is running)
  if (millis() - lastHeartbeat > 2000) {
    if (!motionCooldown) { // Don't interfere with motion indication
      digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    }
    lastHeartbeat = millis();
  }
  
  // Periodic status logging (every 60 seconds)
  if (millis() - lastStatusUpdate > 60000) {
    logger.log(LOG_INFO, "SYSTEM", "Periodic status update", "");
    logger.logSystemStats();
    lastStatusUpdate = millis();
  }
}

// ═══════════════════════════════════════════════════════════
//                    MISSING ESSENTIAL FUNCTIONS
// ═══════════════════════════════════════════════════════════

void setup() {
  initializeSystem();
}

void loop() {
  if (!systemReady) return;
  if (resetRequested) {
    logger.log(LOG_WARNING, "SYSTEM", "Reset button pressed, restarting ESP32", "");
    digitalWrite(STATUS_LED, LOW); 
    for(int i = 0; i < 5; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
    }
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300); // debounce
    
    ESP.restart();
  }
  // LoRa receive handler
  if (LoRa.parsePacket()) {
    String message = "";
    while (LoRa.available()) {
      message += (char)LoRa.read();
    }
    processReceivedPacket(message);
  }
  
  // Check for motion detection
  checkMotionSensor();
  
  // Process queued messages
  processMessageQueue();
  
  // Update system status
  updateSystemStatus();
  
  delay(50); // Optimized loop delay
}

void checkMotionSensor() {
  // Check cooldown period
  if (motionCooldown && (millis() - lastMotionTime > MOTION_DELAY)) {
    motionCooldown = false;
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(STATUS_LED, LOW);
  }
  
  // Read motion sensor
  int motionState = digitalRead(PIR_PIN);
  
  // Detect motion on rising edge (LOW to HIGH)
  if (motionState == HIGH && lastMotionState == LOW && !motionCooldown) {
    handleMotionDetection();
  }
  
  lastMotionState = motionState;
}

void warmupPIRSensor() {
  logger.log(LOG_INFO, "HARDWARE", "Starting PIR sensor warmup", "Duration: " + String(SENSOR_WARMUP/1000) + " seconds");
  
  Serial.println("🔥 Warming up PIR sensor...");
  Serial.print("Please wait ");
  Serial.print(SENSOR_WARMUP/1000);
  Serial.println(" seconds for sensor stabilization");
  
  // Visual feedback during warmup
  for (int i = 0; i < SENSOR_WARMUP/1000; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(STATUS_LED, LOW);
    delay(800);
    
    // Feed watchdog to prevent reset during long warmup
    yield();
    
    Serial.print(".");
    if ((i + 1) % 10 == 0) {
      Serial.print(" ");
      Serial.print(i + 1);
      Serial.println("s");
    }
  }
  
  Serial.println("\n✅ PIR sensor warmed up and ready!");
  logger.logSystemStatus("PIR Sensor", "READY", "Warmup completed successfully");
}

void displaySystemReady() {
  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                        SYSTEM READY                          ║");
  Serial.println("╠══════════════════════════════════════════════════════════════╣");
  Serial.printf("║ Device ID: %-49s ║\n", deviceID.c_str());
  Serial.printf("║ PIR Sensor: %-47s ║\n", "Active");
  Serial.printf("║ LoRa Module: %-46s ║\n", "Connected");
  Serial.printf("║ RTC Module: %-47s ║\n", rtcAvailable ? "Connected" : "Fallback Mode");
  Serial.printf("║ Current Time: %-45s ║\n", getCurrentDateTime().c_str());
  Serial.printf("║ Alert Time: After %-39s ║\n", (String(ALERT_CLOCK_TIME) + ":00").c_str());
  Serial.printf("║ Status: %-51s ║\n", "🟢 MONITORING");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("🚨 Motion detection system is now ACTIVE");
  Serial.println("📡 LoRa transmitter ready for alerts");
  Serial.println("🔍 Monitoring area for motion...");
  if (!rtcAvailable) {
    Serial.println("⚠️  RTC not detected - time filtering disabled");
  } else {
    Serial.printf("🕐 Alerts will be sent after %d:00 (24h format)\n", ALERT_CLOCK_TIME);
  }
  Serial.println();
}

// Enhanced error handling with professional logging
void blinkError() {
  logger.log(LOG_CRITICAL, "SYSTEM", "Critical error - entering error state", "LED blinking pattern active");
  
  while(1) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(STATUS_LED, LOW);
    delay(200);
  }
}

// Add this to your transmitter.ino file in the processReceivedPacket function or equivalent
void processReceivedPacket(String message) {
  // Existing packet reception code...
  if (message.indexOf("\"type\":\"CONFIG\"") > -1) {
    // Parse the JSON message
    // You can use ArduinoJson library if available
    // Simple parsing example (consider using ArduinoJson for more robust parsing)
    if (message.indexOf("\"setting\":\"alert_time\"") > -1) {
      int startIndex = message.indexOf("\"value\":") + 8;
      int endIndex = message.indexOf(",", startIndex);
      String valueStr = message.substring(startIndex, endIndex);
      int alertTime = valueStr.toInt();
      // Update the alert time
      if (alertTime >= 0 && alertTime <= 23) {
        ALERT_CLOCK_TIME = alertTime;
        saveAlertClockTimeToEEPROM(alertTime);
        logger.log(LOG_INFO, "CONFIG", "Alert time updated via LoRa", "New time: " + String(ALERT_CLOCK_TIME) + ":00");
        // Feedback beep
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
        delay(500);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
  }
}

void saveAlertClockTimeToEEPROM(int alertTime) {
  EEPROM.write(EEPROM_ADDR_ALERT_TIME, alertTime);
  EEPROM.commit();
  logger.log(LOG_INFO, "EEPROM", "Saved ALERT_CLOCK_TIME to EEPROM", String(alertTime));
}

int loadAlertClockTimeFromEEPROM() {
  int value = EEPROM.read(EEPROM_ADDR_ALERT_TIME);
  if (value < 0 || value > 23) {
    value = 16; // fallback default
  }
  logger.log(LOG_INFO, "EEPROM", "Loaded ALERT_CLOCK_TIME from EEPROM", String(value));
  return value;
}

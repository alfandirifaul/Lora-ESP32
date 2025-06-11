/*********
  Alfandi Nurhuda - PAMS Security System
  Professional Motion Detection Transmitter v3.0
  Enhanced PIR Motion Sensor with Advanced Logging
*********/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>  // For MAC address

// ═══════════════════════════════════════════════════════════
//                    HARDWARE CONFIGURATION
// ═══════════════════════════════════════════════════════════

// LoRa Module Pin Configuration
#define ss 5
#define rst 14
#define dio0 2

// Sensor & Indicator Configuration
#define PIR_PIN 4         // PIR Motion Sensor GPIO
#define LED_PIN 13        // Status LED GPIO
#define STATUS_LED 2      // Built-in LED (ESP32)

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

// ═══════════════════════════════════════════════════════════
//                    QUEUE STRUCTURE
// ═══════════════════════════════════════════════════════════

struct QueuedMessage {
  String type;
  String deviceId;
  int count;
  unsigned long timestamp;
  String datetime;
  unsigned long uptime;
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

// ═══════════════════════════════════════════════════════════
//                    INITIALIZATION FUNCTIONS
// ═══════════════════════════════════════════════════════════

void initializeSystem() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait max 3 seconds for serial
  
  systemStartTime = millis();
  
  displayStartupBanner();
  Serial.println("🔧 SYSTEM INITIALIZATION");
  Serial.println("════════════════════════════════════════");
  
  // Initialize GPIO pins
  initializeGPIO();
  
  // Initialize LoRa module
  initializeLoRa();
  
  // Initialize device identification
  initializeDeviceID();
  
  // Initialize message queue
  initializeQueue();
  Serial.println("📋 Message queue initialized");
  
  // Warm up PIR sensor
  warmupPIRSensor();
  
  // System ready
  systemReady = true;
  displaySystemReady();
  
  Serial.println("✅ SYSTEM READY - MONITORING ACTIVE");
  Serial.println("════════════════════════════════════════\n");
}

void displayStartupBanner() {
  Serial.println();
  Serial.println("████████████████████████████████████████");
  Serial.println("█            SECURITY SYSTEM           █");
  Serial.println("█     Motion Detection Transmitter     █");
  Serial.println("█              Version 1.0             █");
  Serial.println("████████████████████████████████████████");
  Serial.println();
}

void initializeGPIO() {
  Serial.print("📌 Initializing GPIO pins... ");
  
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
  
  Serial.println("OK");
}

void initializeLoRa() {
  Serial.print("📡 Initializing LoRa module... ");
  
  LoRa.setPins(ss, rst, dio0);
  
  int attempts = 0;
  while (!LoRa.begin(FREQUENCY) && attempts < 10) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (attempts >= 10) {
    Serial.println("FAILED!");
    Serial.println("❌ ERROR: LoRa initialization failed");
    while(1) { 
      blinkError(); 
    }
  }
  
  // Configure LoRa parameters
  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setTxPower(TX_POWER);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  
  Serial.println("OK");
  Serial.print("   • Frequency: ");
  Serial.print(FREQUENCY / 1E6);
  Serial.println(" MHz");
  Serial.print("   • TX Power: ");
  Serial.print(TX_POWER);
  Serial.println(" dBm");
  Serial.print("   • Sync Word: 0x");
  Serial.println(SYNC_WORD, HEX);
}

void initializeDeviceID() {
  Serial.print("🆔 Generating device ID... ");
  
  // Get MAC address for unique ID
  WiFi.mode(WIFI_STA);
  String mac = WiFi.macAddress();
  deviceID = "TX-" + mac.substring(12); // Last 6 characters
  deviceID.replace(":", "");
  
  Serial.print("OK (");
  Serial.print(deviceID);
  Serial.println(")");
}

void warmupPIRSensor() {
  Serial.print("🌡️  Warming up PIR sensor");
  
  digitalWrite(LED_PIN, HIGH);
  
  for (int i = 0; i < SENSOR_WARMUP / 1000; i++) {
    Serial.print(".");
    delay(1000);
  }
  
  digitalWrite(LED_PIN, LOW);
  Serial.println(" OK");
  Serial.println("   • Sensor stabilized and ready");
}

void displaySystemReady() {
  // Ready animation
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(STATUS_LED, LOW);
    delay(100);
  }
}

// ═══════════════════════════════════════════════════════════
//                    MAIN PROGRAM FUNCTIONS
// ═══════════════════════════════════════════════════════════

void setup() {
  initializeSystem();
}

void loop() {
  if (!systemReady) return;
  
  // Check for motion detection
  checkMotionSensor();
  
  // Process queued messages
  processMessageQueue();
  
  // Update system status
  updateSystemStatus();
  
  delay(50); // Optimized loop delay
}

// ═══════════════════════════════════════════════════════════
//                    QUEUE MANAGEMENT FUNCTIONS
// ═══════════════════════════════════════════════════════════

void initializeQueue() {
  // Initialize all queue entries as invalid
  for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
    messageQueue[i].isValid = false;
    messageQueue[i].retryCount = 0;
  }
  queueHead = 0;
  queueTail = 0;
  queueSize = 0;
}

bool enqueueMessage(String type, String deviceId, int count, unsigned long timestamp, String datetime, unsigned long uptime) {
  if (queueSize >= MAX_QUEUE_SIZE) {
    Serial.println("⚠️  Queue full! Dropping oldest message...");
    dequeueMessage(); // Remove oldest message
  }
  
  messageQueue[queueTail].type = type;
  messageQueue[queueTail].deviceId = deviceId;
  messageQueue[queueTail].count = count;
  messageQueue[queueTail].timestamp = timestamp;
  messageQueue[queueTail].datetime = datetime;
  messageQueue[queueTail].uptime = uptime;
  messageQueue[queueTail].retryCount = 0;
  messageQueue[queueTail].isValid = true;
  
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueSize++;
  queuedMessages++;
  
  Serial.print("📥 Message queued (Queue size: ");
  Serial.print(queueSize);
  Serial.println(")");
  
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
  
  // Check if it's time to retry
  if (millis() - lastRetryTime < RETRY_INTERVAL) return;
  
  // Try to send the oldest message in queue
  if (messageQueue[queueHead].isValid) {
    bool success = transmitQueuedMessage(queueHead);
    
    if (success) {
      Serial.println("✅ Queued message sent successfully");
      dequeueMessage();
    } else {
      messageQueue[queueHead].retryCount++;
      retriedMessages++;
      
      if (messageQueue[queueHead].retryCount >= MAX_RETRY_ATTEMPTS) {
        Serial.print("❌ Message failed after ");
        Serial.print(MAX_RETRY_ATTEMPTS);
        Serial.println(" attempts. Dropping...");
        dequeueMessage();
        transmissionErrors++;
      } else {
        Serial.print("🔄 Retry ");
        Serial.print(messageQueue[queueHead].retryCount);
        Serial.print("/");
        Serial.print(MAX_RETRY_ATTEMPTS);
        Serial.println(" failed. Will retry...");
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
  LoRa.print("\"uptime\":");
  LoRa.print(msg.uptime);
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
//                    ENHANCED MOTION DETECTION
// ═══════════════════════════════════════════════════════════

void checkMotionSensor() {
  // Check cooldown period
  if (motionCooldown && (millis() - lastMotionTime > MOTION_DELAY)) {
    motionCooldown = false;
    digitalWrite(LED_PIN, LOW);
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

bool sendMotionAlert() {
  Serial.println("📤 Transmitting motion alert...");
  
  String currentDateTime = getCurrentDateTime();
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
  LoRa.print("\"uptime\":");
  LoRa.print(uptime);
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
  // Simple timestamp format since we don't have RTC
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = (totalSeconds / 3600) % 24;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long seconds = totalSeconds % 60;
  
  String datetime = "T+";
  if (hours < 10) datetime += "0";
  datetime += String(hours);
  datetime += ":";
  if (minutes < 10) datetime += "0";
  datetime += String(minutes);
  datetime += ":";
  if (seconds < 10) datetime += "0";
  datetime += String(seconds);
  
  return datetime;
}

void handleMotionDetection() {
  motionCounter++;
  lastMotionTime = millis();
  motionCooldown = true;
  
  // Visual indication
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(STATUS_LED, HIGH);
  
  // Try immediate transmission first
  bool success = sendMotionAlert();
  
  // If transmission fails, queue the message
  if (!success) {
    String currentDateTime = getCurrentDateTime();
    unsigned long uptime = (millis() - systemStartTime) / 1000;
    
    enqueueMessage("MOTION", deviceID, motionCounter, millis(), currentDateTime, uptime);
    Serial.println("📋 Motion alert queued for retry");
  }
  
  // Log the detection
  logMotionDetection(success);
}

void logMotionDetection(bool success) {
  unsigned long uptime = (millis() - systemStartTime) / 1000;
  
  Serial.print("📊 Alert #");
  Serial.print(motionCounter);
  Serial.print(" | Device: ");
  Serial.println(deviceID);
  
  Serial.print("🕐 Timestamp: ");
  Serial.print(uptime);
  Serial.print("s | Status: ");
  Serial.println(success ? "✅ SENT" : "📋 QUEUED");
  
  Serial.print("📈 Stats: ");
  Serial.print(packetsSent);
  Serial.print(" sent, ");
  Serial.print(transmissionErrors);
  Serial.print(" errors, ");
  Serial.print(queueSize);
  Serial.print(" queued, ");
  Serial.print(retriedMessages);
  Serial.println(" retries");
  
  Serial.println("────────────────────────────────────────\n");
}

// ═══════════════════════════════════════════════════════════
//                    SYSTEM MONITORING
// ═══════════════════════════════════════════════════════════

void updateSystemStatus() {
  // Blink status LED to show system is alive
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    lastBlink = millis();
  }
}

// ═══════════════════════════════════════════════════════════
//                    ERROR HANDLING
// ═══════════════════════════════════════════════════════════

void blinkError() {
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(STATUS_LED, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
  delay(200);
}
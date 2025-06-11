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
#define STATUS_LED 12      // Built-in LED (ESP32)

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

// Add these variables for better status tracking
unsigned long lastStatusRequest = 0;
unsigned long statusRequestInterval = 10000; // Request status every 10 seconds if no updates

// ═══════════════════════════════════════════════════════════
//                    RECEIVER STATE & QUEUE MANAGEMENT
// ═══════════════════════════════════════════════════════════

// Receiver state tracking
struct ReceiverState {
  bool isOnline = false;
  bool isReady = false;
  bool isBusy = false;
  bool alarmActive = false;
  unsigned long lastStatusReceived = 0;
  String receiverID = "";
} receiverState;

// Message queue structure
struct MessageQueue {
  String messages[10];  // Store up to 10 messages
  int count = 0;
  int front = 0;
  int rear = 0;
} messageQueue;

// Queue management variables
unsigned long lastQueueCheck = 0;
unsigned long queueCheckInterval = 1000;  // Check queue every 1 second
bool waitingForReceiver = false;

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
  
  // Enable listening mode for status updates
  LoRa.receive();
  
  Serial.println("OK");
  Serial.print("   • Frequency: ");
  Serial.print(FREQUENCY / 1E6);
  Serial.println(" MHz");
  Serial.print("   • TX Power: ");
  Serial.print(TX_POWER);
  Serial.println(" dBm");
  Serial.print("   • Sync Word: 0x");
  Serial.println(SYNC_WORD, HEX);
  Serial.println("   • Listening mode enabled");
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
  
  // Priority 1: Process receiver status updates
  processReceiverStatus();
  
  // Priority 2: Check for motion detection
  checkMotionSensor();
  
  // Priority 3: Process message queue when receiver is ready
  if (millis() - lastQueueCheck > queueCheckInterval) {
    if (messageQueue.count > 0 && receiverState.isReady && !receiverState.isBusy && !receiverState.alarmActive) {
      processMessageQueue();
    }
    lastQueueCheck = millis();
  }
  
  // Priority 4: Request status if no recent updates
  requestReceiverStatus();
  
  // Priority 5: Check if receiver is offline
  if (millis() - receiverState.lastStatusReceived > 30000) {
    if (receiverState.isOnline) {
      Serial.println("⚠️ Receiver appears to be offline - marking as unavailable");
      receiverState.isOnline = false;
      receiverState.isReady = false;
    }
  }
  
  // Priority 6: Update system status
  updateSystemStatus();
  
  delay(50); // Optimized loop delay
}

// ═══════════════════════════════════════════════════════════
//                    MOTION DETECTION SYSTEM
// ═══════════════════════════════════════════════════════════

void checkMotionSensor() {
  int currentMotionState = digitalRead(PIR_PIN);
  
  // Detect motion (LOW to HIGH transition)
  if (currentMotionState == HIGH && lastMotionState == LOW) {
    if (!motionCooldown) {
      handleMotionDetection();
    }
  }
  
  // Check cooldown period
  if (motionCooldown && (millis() - lastMotionTime) > MOTION_DELAY) {
    endMotionCooldown();
  }
  
  lastMotionState = currentMotionState;
}

void handleMotionDetection() {
  motionCounter++;
  lastMotionTime = millis();
  motionCooldown = true;
  
  // Visual indication
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(STATUS_LED, HIGH);
  
  // Send motion alert
  bool success = sendMotionAlert();
  
  // Log the detection
  logMotionDetection(success);
}

bool sendMotionAlert() {
  Serial.println("🚨 MOTION DETECTED - PREPARING ALERT");
  Serial.println("────────────────────────────────────────");
  
  // Get current date/time
  String currentDateTime = getCurrentDateTime();
  
  // Create structured message with priority flag
  String alertMessage = "{";
  alertMessage += "\"type\":\"MOTION\",";
  alertMessage += "\"id\":\"" + deviceID + "\",";
  alertMessage += "\"count\":" + String(motionCounter) + ",";
  alertMessage += "\"time\":" + String(millis()) + ",";
  alertMessage += "\"datetime\":\"" + currentDateTime + "\",";
  alertMessage += "\"uptime\":" + String((millis() - systemStartTime) / 1000) + ",";
  alertMessage += "\"priority\":\"HIGH\"";
  alertMessage += "}";
  
  // Check receiver status - be more specific about why we're queuing
  if (!receiverState.isOnline) {
    Serial.println("⚠️ Receiver OFFLINE - adding to queue");
    addToQueue(alertMessage);
    waitingForReceiver = true;
    return false;
  }
  
  if (receiverState.isBusy) {
    Serial.println("⏸️ Receiver BUSY processing - adding to queue");
    addToQueue(alertMessage);
    waitingForReceiver = true;
    return false;
  }
  
  if (receiverState.alarmActive) {
    Serial.println("🚨 Receiver ALARM ACTIVE - adding to queue");
    addToQueue(alertMessage);
    waitingForReceiver = true;
    return false;
  }
  
  if (!receiverState.isReady) {
    Serial.println("⏳ Receiver NOT READY - adding to queue");
    addToQueue(alertMessage);
    waitingForReceiver = true;
    return false;
  }
  
  // Receiver is ready, send immediately
  Serial.println("📡 Receiver READY - transmitting immediately");
  
  LoRa.beginPacket();
  LoRa.print(alertMessage);
  bool success = LoRa.endPacket();
  
  if (success) {
    packetsSent++;
    Serial.println("✅ Alert transmitted successfully");
  } else {
    transmissionErrors++;
    Serial.println("❌ Transmission failed - adding to queue for retry");
    addToQueue(alertMessage);
    waitingForReceiver = true;
  }
  
  // Resume listening
  LoRa.receive();
  
  return success;
}

String getCurrentDateTime() {
  // Calculate elapsed time since system start
  unsigned long totalSeconds = (millis() - systemStartTime) / 1000;
  
  // Convert to days, hours, minutes, seconds
  int days = totalSeconds / 86400;
  totalSeconds %= 86400;
  int hours = totalSeconds / 3600;
  totalSeconds %= 3600;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  
  // Format as readable date/time string
  String dateTime = "Day" + String(days + 1) + " ";
  
  if (hours < 10) dateTime += "0";
  dateTime += String(hours) + ":";
  
  if (minutes < 10) dateTime += "0";
  dateTime += String(minutes) + ":";
  
  if (seconds < 10) dateTime += "0";
  dateTime += String(seconds);
  
  return dateTime;
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
  Serial.println(success ? "✅ SENT" : "❌ FAILED");
  
  Serial.print("📈 Stats: ");
  Serial.print(packetsSent);
  Serial.print(" sent, ");
  Serial.print(transmissionErrors);
  Serial.println(" errors");
  
  Serial.println("────────────────────────────────────────\n");
}

void endMotionCooldown() {
  motionCooldown = false;
  digitalWrite(LED_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
  
  Serial.println("🔄 Motion sensor ready for next detection");
  Serial.println();
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

// Enhanced processReceiverStatus function
void processReceiverStatus() {
  if (LoRa.parsePacket() == 0) return;
  
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  
  // Resume listening
  LoRa.receive();
  
  // Parse status message (simple JSON parsing)
  if (message.indexOf("\"type\":\"STATUS\"") > -1) {
    receiverState.lastStatusReceived = millis();
    receiverState.isOnline = true;
    
    // Extract receiver ID
    int idStart = message.indexOf("\"id\":\"") + 6;
    int idEnd = message.indexOf("\"", idStart);
    if (idStart > 5 && idEnd > idStart) {
      receiverState.receiverID = message.substring(idStart, idEnd);
    }
    
    // Parse receiver state
    receiverState.isBusy = message.indexOf("\"busy\":true") > -1;
    receiverState.alarmActive = message.indexOf("\"alarm\":true") > -1;
    receiverState.isReady = message.indexOf("\"ready\":true") > -1;
    
    Serial.println("📡 Receiver status update received:");
    Serial.printf("   • ID: %s\n", receiverState.receiverID.c_str());
    Serial.printf("   • Online: %s\n", receiverState.isOnline ? "YES" : "NO");
    Serial.printf("   • Busy: %s\n", receiverState.isBusy ? "YES" : "NO");
    Serial.printf("   • Alarm: %s\n", receiverState.alarmActive ? "YES" : "NO");
    Serial.printf("   • Ready: %s\n", receiverState.isReady ? "YES" : "NO");
    
    // If receiver is ready and we have queued messages, process them immediately
    if (receiverState.isReady && messageQueue.count > 0) {
      Serial.println("🚀 Receiver is ready - processing queue immediately");
      processMessageQueue();
    }
  }
  
  // Handle status request responses
  else if (message.indexOf("\"type\":\"STATUS_REQUEST\"") > -1) {
    Serial.println("📞 Status request acknowledged by receiver");
  }
}

// Add status request function
void requestReceiverStatus() {
  if (millis() - lastStatusRequest < statusRequestInterval) {
    return;
  }
  
  // Only request if we haven't heard from receiver recently
  if (millis() - receiverState.lastStatusReceived > 15000) {
    String requestMessage = "{\"type\":\"STATUS_REQUEST\",\"id\":\"" + deviceID + "\"}";
    
    LoRa.beginPacket();
    LoRa.print(requestMessage);
    bool success = LoRa.endPacket();
    
    if (success) {
      Serial.println("📞 Status request sent to receiver");
    }
    
    LoRa.receive();
  }
  
  lastStatusRequest = millis();
}

// ═══════════════════════════════════════════════════════════
//                    MESSAGE QUEUE FUNCTIONS
// ═══════════════════════════════════════════════════════════

void addToQueue(String message) {
  if (messageQueue.count >= 10) {
    Serial.println("⚠️ Queue full - dropping oldest message");
    // Remove oldest message
    messageQueue.front = (messageQueue.front + 1) % 10;
    messageQueue.count--;
  }
  
  messageQueue.messages[messageQueue.rear] = message;
  messageQueue.rear = (messageQueue.rear + 1) % 10;
  messageQueue.count++;
  
  Serial.printf("📥 Message queued (Queue: %d/10)\n", messageQueue.count);
}

void processMessageQueue() {
  if (messageQueue.count == 0) return;
  
  Serial.printf("🚀 Processing queue - %d messages pending\n", messageQueue.count);
  
  while (messageQueue.count > 0 && receiverState.isReady && !receiverState.isBusy && !receiverState.alarmActive) {
    String message = messageQueue.messages[messageQueue.front];
    messageQueue.front = (messageQueue.front + 1) % 10;
    messageQueue.count--;
    
    Serial.println("📤 Sending queued message...");
    
    LoRa.beginPacket();
    LoRa.print(message);
    bool success = LoRa.endPacket();
    
    if (success) {
      packetsSent++;
      Serial.printf("✅ Queued message sent successfully (Queue: %d/10)\n", messageQueue.count);
    } else {
      // Put message back in queue if failed
      transmissionErrors++;
      Serial.println("❌ Failed to send queued message - adding back to queue");
      
      // Add back to front of queue
      messageQueue.front = (messageQueue.front - 1 + 10) % 10;
      messageQueue.messages[messageQueue.front] = message;
      messageQueue.count++;
      break; // Stop processing if transmission fails
    }
    
    // Resume listening after each transmission
    LoRa.receive();
    
    // Small delay between queue transmissions
    delay(100);
  }
  
  if (messageQueue.count == 0) {
    waitingForReceiver = false;
    Serial.println("✅ All queued messages processed successfully");
  }
}
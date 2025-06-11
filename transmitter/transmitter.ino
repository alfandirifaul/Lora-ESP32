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
  
  // Update system status
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
  Serial.println("🚨 MOTION DETECTED - TRANSMITTING ALERT");
  Serial.println("────────────────────────────────────────");
  
  LoRa.beginPacket();
  
  // Get current date/time
  String currentDateTime = getCurrentDateTime();
  
  // Create structured message
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
  LoRa.print((millis() - systemStartTime) / 1000);
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
/*********
  Alfandi Nurhuda - pams
  Modified from the examples of the Arduino LoRa library
*********/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2

#define BUZZER 12

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Emergency alarm settings
const int ALARM_DURATION = 5000;    // Total alarm duration (5 seconds)
const int BEEP_ON_TIME = 200;       // Beep on time (200ms)
const int BEEP_OFF_TIME = 200;      // Beep off time (200ms)
const int ALARM_FREQUENCY = 1000;   // Buzzer frequency (1kHz)

// Variables for alarm control
bool alarmActive = false;
unsigned long alarmStartTime = 0;
unsigned long lastBeepTime = 0;
bool buzzerState = false;

// Motion detection log
int motionCount = 0;
unsigned long lastMotionTime = 0;
String lastRSSI = "";
bool displayNeedsUpdate = true;

void setup() {  
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver with Emergency Alarm & OLED");

  // Initialize OLED display - try different address
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Changed from 0x3C to 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    // Don't halt, continue for debugging
    Serial.println("Continuing without OLED...");
  } else {
    Serial.println("OLED initialized successfully!");
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("LoRa Motion Detector");
  display.println("Initializing...");
  display.display();

  // Initialize buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW); // Turn off buzzer initially

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    display.print(".");
    display.display();
    delay(500);
  }
   
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  
  // Display ready status
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("LoRa Motion Detector");
  display.println("Status: READY");
  display.println("Waiting for motion...");
  display.println("");
  display.println("Motion Count: 0");
  display.display();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  
  // Header
  display.println("LoRa Motion Detector");
  display.println("--------------------");
  
  // Status
  if (alarmActive) {
    display.setTextSize(2);
    display.println("MOTION!");
    display.setTextSize(1);
  } else {
    display.println("Status: Monitoring");
  }
  
  // Motion count
  display.print("Count: ");
  display.println(motionCount);
  
  // Last detection time
  if (motionCount > 0) {
    display.print("Last: ");
    unsigned long timeSince = (millis() - lastMotionTime) / 1000;
    display.print(timeSince);
    display.println("s ago");
    
    // RSSI
    display.print("RSSI: ");
    display.print(lastRSSI);
    display.println(" dBm");
  }
  
  display.display();
}

void startEmergencyAlarm() {
  alarmActive = true;
  alarmStartTime = millis();
  lastBeepTime = millis();
  buzzerState = true;
  digitalWrite(BUZZER, HIGH);
  Serial.println("🚨 EMERGENCY ALARM ACTIVATED! 🚨");
  displayNeedsUpdate = true;
}

void updateEmergencyAlarm() {
  if (!alarmActive) return;
  
  unsigned long currentTime = millis();
  
  // Check if alarm duration has ended
  if (currentTime - alarmStartTime >= ALARM_DURATION) {
    alarmActive = false;
    buzzerState = false;
    digitalWrite(BUZZER, LOW);
    Serial.println("Emergency alarm deactivated");
    displayNeedsUpdate = true;
    return;
  }
  
  // Handle beeping pattern
  if (buzzerState) {
    // Buzzer is currently ON, check if it's time to turn OFF
    if (currentTime - lastBeepTime >= BEEP_ON_TIME) {
      buzzerState = false;
      digitalWrite(BUZZER, LOW);
      lastBeepTime = currentTime;
    }
  } else {
    // Buzzer is currently OFF, check if it's time to turn ON
    if (currentTime - lastBeepTime >= BEEP_OFF_TIME) {
      buzzerState = true;
      digitalWrite(BUZZER, HIGH);
      lastBeepTime = currentTime;
    }
  }
}

void loop() {
  // Update emergency alarm if active
  updateEmergencyAlarm();
  
  // Update display if needed
  if (displayNeedsUpdate) {
    updateDisplay();
    displayNeedsUpdate = false;
  }
  
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Update motion detection data
    motionCount++;
    lastMotionTime = millis();
    lastRSSI = String(LoRa.packetRssi());
    
    // Start emergency alarm when packet is received
    startEmergencyAlarm();
    
    // received a packet
    Serial.print("📡 MOTION DETECTED! Received packet: '");

    // read packet
    String receivedData = "";
    while (LoRa.available()) {
      receivedData = LoRa.readString();
      Serial.print(receivedData); 
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.print(LoRa.packetRssi());
    Serial.println(" dBm");
    
    displayNeedsUpdate = true;
  }
  
  // Update display every 5 seconds when not in alarm
  static unsigned long lastDisplayUpdate = 0;
  if (!alarmActive && (millis() - lastDisplayUpdate) > 5000) {
    displayNeedsUpdate = true;
    lastDisplayUpdate = millis();
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}
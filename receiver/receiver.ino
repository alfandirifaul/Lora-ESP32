/*********
  Alfandi Nurhuda - pams
  Modified from the examples of the Arduino LoRa library
*********/

#include <SPI.h>
#include <LoRa.h>

//define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2

#define BUZZER 12

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

void setup() {  
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver with Emergency Alarm");

  // Initialize buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW); // Turn off buzzer initially

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
   
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void startEmergencyAlarm() {
  alarmActive = true;
  alarmStartTime = millis();
  lastBeepTime = millis();
  buzzerState = true;
  digitalWrite(BUZZER, HIGH);
  Serial.println("🚨 EMERGENCY ALARM ACTIVATED! 🚨");
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
  
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Start emergency alarm when packet is received
    startEmergencyAlarm();
    
    // received a packet
    Serial.print("📡 MOTION DETECTED! Received packet: '");

    // read packet
    while (LoRa.available()) {
      String LoRaData = LoRa.readString();
      Serial.print(LoRaData); 
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.print(LoRa.packetRssi());
    Serial.println(" dBm");
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}
/*********
  Alfandi Nurhuda - pams
  Modified from the examples of the Arduino LoRa library
  Updated to use PIR motion sensor
*********/

#include <SPI.h>
#include <LoRa.h>

// Define pins for LoRa transceiver
#define ss 5
#define rst 14
#define dio0 2

// PIR sensor configuration
#define PIR_PIN 4         // GPIO pin for the PIR sensor
#define LED_PIN 13        // LED indicator pin (optional)

// Motion detection settings
const unsigned long MOTION_DELAY = 2000; // Minimum time between motion detections (2 seconds)

// Variables for motion detection
int lastMotionState = LOW;           // Last motion sensor state
unsigned long lastMotionTime = 0;    // Last time motion was detected
bool motionCooldown = false;         // Prevents multiple rapid detections

int counter = 0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Sender with PIR Motion Sensor");

  // Initialize PIR sensor pin as input
  pinMode(PIR_PIN, INPUT);
  
  // Initialize LED pin as output (optional indicator)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  // Frequency setup (choose one):
  while (!LoRa.begin(433E6)) {   // 433MHz for Asia
  // while (!LoRa.begin(868E6)) { // 868MHz for Europe
  // while (!LoRa.begin(915E6)) { // 915MHz for North America
    Serial.println(".");
    delay(500);
  }
  
  // Sync word setup (must match receiver)
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  
  // PIR sensor warm-up time
  Serial.println("PIR sensor warming up...");
  digitalWrite(LED_PIN, HIGH);
  delay(2000); // Give PIR sensor time to stabilize
  digitalWrite(LED_PIN, LOW);
  Serial.println("PIR sensor ready!");
  
  // System ready indication
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

void loop() {
  // Read current PIR sensor state
  int currentMotionState = digitalRead(PIR_PIN);
  
  // Check for motion detection (LOW to HIGH transition)
  if (currentMotionState == HIGH && lastMotionState == LOW) {
    // Check if we're not in cooldown period
    if (!motionCooldown) {
      Serial.print("Motion detected! Sending packet: ");
      Serial.println(counter);

      // Turn on LED to indicate motion detection
      digitalWrite(LED_PIN, HIGH);

      // Send LoRa packet with motion alert
      LoRa.beginPacket();
      LoRa.print("Motion Alert #");
      LoRa.print(counter);
      LoRa.print(" detected at ");
      LoRa.print(millis());
      LoRa.print("ms");
      LoRa.endPacket();

      counter++;
      
      // Start cooldown period
      motionCooldown = true;
      lastMotionTime = millis();
      
      Serial.println("Packet sent successfully!");
    }
  }
  
  // Check if cooldown period has ended
  if (motionCooldown && (millis() - lastMotionTime) > MOTION_DELAY) {
    motionCooldown = false;
    digitalWrite(LED_PIN, LOW); // Turn off LED
    Serial.println("Motion sensor ready for next detection");
  }
  
  // Update last motion state
  lastMotionState = currentMotionState;
  
  // Small delay to reduce CPU load
  delay(100);
}
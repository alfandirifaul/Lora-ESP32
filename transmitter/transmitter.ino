/*********
  Alfandi Nurhuda - pams
  Modified from the examples of the Arduino LoRa library
*********/

#include <SPI.h>
#include <LoRa.h>

// Define pins for LoRa transceiver
#define ss 5
#define rst 14
#define dio0 2

// Button configuration
#define BUTTON_PIN 4      // GPIO pin for the button

// Debounce settings
const unsigned long DEBOUNCE_DELAY = 50; // Debounce time in milliseconds

// Variables for button debouncing
int lastSteadyState = HIGH;      // Last stable button state
int lastFlickerableState = HIGH; // Last raw button state
unsigned long lastDebounceTime = 0; // Last debounce time

int counter = 0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Sender with Debounced Button");

  // Initialize button pin with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);  

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
}

void loop() {
  // Read current button state
  int currentState = digitalRead(BUTTON_PIN);
  
  // Debounce logic
  if (currentState != lastFlickerableState) {
    // Reset debounce timer
    lastDebounceTime = millis();
    lastFlickerableState = currentState;
  }

  // Check if debounce delay has passed
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Only process if state changed
    if (lastSteadyState == HIGH && currentState == LOW) {
      Serial.print("Button pressed! Sending packet: ");
      Serial.println(counter);

      // Send LoRa packet
      LoRa.beginPacket();
      LoRa.print("Hello World! Message #");
      LoRa.print(counter);
      LoRa.endPacket();

      counter++;
    }
    lastSteadyState = currentState;
  }
  
  // Small delay to reduce CPU load
  delay(10);
}
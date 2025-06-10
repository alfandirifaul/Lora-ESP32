/*********
  Alfandi Nurhuda - pams
  Modified from the examples of the Arduino LoRa library
  Enhanced Motion Detection Data Logger with Professional Display
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
const int ALARM_DURATION = 3000;  // Total alarm duration (3 seconds)
const int BEEP_ON_TIME = 150;     // Beep on time (150ms)
const int BEEP_OFF_TIME = 150;    // Beep off time (150ms)

// Variables for alarm control
bool alarmActive = false;
unsigned long alarmStartTime = 0;
unsigned long lastBeepTime = 0;
bool buzzerState = false;

// Enhanced motion detection log
int motionCount = 0;
unsigned long lastMotionTime = 0;
String lastRSSI = "";
String lastMessage = "";
bool displayNeedsUpdate = true;
unsigned long systemStartTime = 0;
int maxRSSI = -999;
int minRSSI = 999;

// Display animation variables
int animationFrame = 0;
unsigned long lastAnimationTime = 0;

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("🚀 Advanced LoRa Motion Detection System");
  Serial.println("=========================================");

  // Record system start time
  systemStartTime = millis();

  // Initialize I2C and OLED display
  Wire.begin();
  Serial.println("📺 Initializing OLED Display...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ SSD1306 allocation failed at 0x3C");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("❌ SSD1306 allocation failed at 0x3D");
      Serial.println("⚠️  Continuing without OLED display...");
    } else {
      Serial.println("✅ OLED found at address 0x3D!");
    }
  } else {
    Serial.println("✅ OLED found at address 0x3C!");
  }

  // Startup animation
  showStartupScreen();

  // Initialize buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  //setup LoRa transceiver module
  Serial.println("📡 Initializing LoRa Module...");
  LoRa.setPins(ss, rst, dio0);

  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    showInitializingScreen();
    delay(500);
  }

  LoRa.setSyncWord(0xF3);
  Serial.println("✅ LoRa Module Ready!");

  // Show ready screen
  showReadyScreen();
  Serial.println("🎯 System Ready - Monitoring for Motion...");
}

void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);
  display.println("MOTION");
  display.setCursor(5, 30);
  display.println("DETECTOR");
  display.setTextSize(1);
  display.setCursor(40, 50);
  display.println("v2.0");
  display.display();
  delay(2000);
}

void showInitializingScreen() {
  static int dots = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println("Initializing");
  display.setCursor(25, 35);
  display.print("LoRa Module");
  for (int i = 0; i < dots; i++) {
    display.print(".");
  }
  display.display();
  dots = (dots + 1) % 4;
}

void showReadyScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MOTION DETECTION SYSTEM");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 15);
  display.println("Status: READY");
  display.setCursor(0, 25);
  display.println("Frequency: 433MHz");
  display.setCursor(0, 35);
  display.println("Range: Long Distance");
  display.setCursor(0, 45);
  display.println("Mode: Monitoring");
  display.display();
  delay(2000);
}

void updateDisplay() {
  display.clearDisplay();

  // Header with animation
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("MOTION LOGGER ");

  // Animated status indicator
  if (alarmActive) {
    display.print("🚨");
  } else {
    const char* indicators[] = { "●", "◐", "○", "◑" };
    display.print(indicators[animationFrame]);
  }

  // Draw header line
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // Main content area
  display.setCursor(0, 13);

  if (alarmActive) {
    // Alert mode display
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.println("MOTION!");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print("Alert #");
    display.println(motionCount);
    display.setCursor(0, 50);
    display.print("RSSI: ");
    display.print(lastRSSI);
    display.println(" dBm");
  } else {
    // Normal monitoring display
    display.setTextSize(1);

    // Statistics section
    display.setCursor(0, 13);
    display.print("Detections: ");
    display.println(motionCount);

    display.setCursor(0, 23);
    display.print("Uptime: ");
    unsigned long uptime = (millis() - systemStartTime) / 1000;
    if (uptime < 60) {
      display.print(uptime);
      display.println("s");
    } else if (uptime < 3600) {
      display.print(uptime / 60);
      display.print("m ");
      display.print(uptime % 60);
      display.println("s");
    } else {
      display.print(uptime / 3600);
      display.print("h ");
      display.print((uptime % 3600) / 60);
      display.println("m");
    }

    if (motionCount > 0) {
      // Last detection info
      display.setCursor(0, 33);
      display.print("Last: ");
      unsigned long timeSince = (millis() - lastMotionTime) / 1000;
      if (timeSince < 60) {
        display.print(timeSince);
        display.println("s ago");
      } else {
        display.print(timeSince / 60);
        display.print("m ");
        display.print(timeSince % 60);
        display.println("s ago");
      }

      // Signal strength bar
      display.setCursor(0, 43);
      display.print("Signal: ");
      display.print(lastRSSI);
      display.println(" dBm");

      // RSSI bar graph
      display.setCursor(0, 53);
      int rssiValue = lastRSSI.toInt();
      int barLength = map(constrain(rssiValue, -120, -30), -120, -30, 0, 80);
      display.print("[");
      for (int i = 0; i < 10; i++) {
        if (i < barLength / 8) {
          display.print("=");
        } else {
          display.print("-");
        }
      }
      display.print("]");
    } else {
      // No detections yet
      display.setCursor(20, 40);
      display.println("Waiting for");
      display.setCursor(30, 50);
      display.println("motion...");
    }
  }

  display.display();
}

void startEmergencyAlarm() {
  alarmActive = true;
  alarmStartTime = millis();
  lastBeepTime = millis();
  buzzerState = true;
  digitalWrite(BUZZER, HIGH);

  // Update RSSI statistics
  int currentRSSI = lastRSSI.toInt();
  if (currentRSSI > maxRSSI) maxRSSI = currentRSSI;
  if (currentRSSI < minRSSI) minRSSI = currentRSSI;

  Serial.println("🚨 MOTION ALERT ACTIVATED! 🚨");
  Serial.print("📊 Detection #");
  Serial.print(motionCount);
  Serial.print(" | RSSI: ");
  Serial.print(lastRSSI);
  Serial.println(" dBm");

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
    Serial.println("✅ Emergency alarm deactivated");
    Serial.println();
    Serial.println();

    displayNeedsUpdate = true;
    return;
  }

  // Handle beeping pattern
  if (buzzerState) {
    if (currentTime - lastBeepTime >= BEEP_ON_TIME) {
      buzzerState = false;
      digitalWrite(BUZZER, LOW);
      lastBeepTime = currentTime;
    }
  } else {
    if (currentTime - lastBeepTime >= BEEP_OFF_TIME) {
      buzzerState = true;
      digitalWrite(BUZZER, HIGH);
      lastBeepTime = currentTime;
    }
  }
}

void updateAnimation() {
  if (millis() - lastAnimationTime > 500) {
    animationFrame = (animationFrame + 1) % 4;
    lastAnimationTime = millis();
    if (!alarmActive) displayNeedsUpdate = true;
  }
}

void loop() {
  // Update systems
  updateEmergencyAlarm();
  updateAnimation();

  // Update display if needed
  if (displayNeedsUpdate) {
    updateDisplay();
    displayNeedsUpdate = false;
  }

  // Check for incoming packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Update motion detection data
    motionCount++;
    lastMotionTime = millis();
    lastRSSI = String(LoRa.packetRssi());

    // Read packet data
    lastMessage = "";
    while (LoRa.available()) {
      lastMessage = LoRa.readString();
    }

    // Start emergency alarm
    startEmergencyAlarm();

    // Enhanced serial logging
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.print("📡 MOTION DETECTED | Alert #");
    Serial.println(motionCount);
    Serial.print("📨 Message: ");
    Serial.println(lastMessage);
    Serial.print("📶 RSSI: ");
    Serial.print(lastRSSI);
    Serial.println(" dBm");
    Serial.print("🕐 Time: ");
    Serial.print((millis() - systemStartTime) / 1000);
    Serial.println(" seconds since start");
    if (motionCount > 1) {
      Serial.print("📊 Range: ");
      Serial.print(minRSSI);
      Serial.print(" to ");
      Serial.print(maxRSSI);
      Serial.println(" dBm");
    }
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    displayNeedsUpdate = true;
  }

  // Periodic display refresh (every 10 seconds when not in alarm)
  static unsigned long lastDisplayUpdate = 0;
  if (!alarmActive && (millis() - lastDisplayUpdate) > 10000) {
    displayNeedsUpdate = true;
    lastDisplayUpdate = millis();
  }

  // Small delay for system stability
  delay(10);
}
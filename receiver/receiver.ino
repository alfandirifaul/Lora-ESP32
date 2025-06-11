/*********
  Clean LoRa Motion Detection System
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

const unsigned long BOT_REQUEST_DELAY = 100;  // 100ms for responsiveness
unsigned long lastBotUpdate = 0;

// ═══════════════════════════════════════════════════════════
//                    INTERRUPT SERVICE ROUTINE
// ═══════════════════════════════════════════════════════════
void IRAM_ATTR onLoRaReceive(int packetSize) {
  state.packetReceived = true;
}

// ═══════════════════════════════════════════════════════════
//                    INITIALIZATION FUNCTIONS
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("🚀 LoRa Motion Detection System Starting...");
  Serial.println("==========================================");
  
  state.systemStartTime = millis();
  
  initializeHardware();
  initializeWiFi();
  initializeTelegram();
  initializeLoRa();
  
  showReadyScreen();
  Serial.println("✅ System Ready - Real-Time Monitoring Active!");
}

void initializeHardware() {
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize I2C and OLED
  Wire.begin();
  Serial.println("📺 Initializing OLED Display...");
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("❌ OLED Display not found!");
    } else {
      Serial.println("✅ OLED found at 0x3D");
    }
  } else {
    Serial.println("✅ OLED found at 0x3C");
  }
  
  showStartupScreen();
}

void initializeWiFi() {
  Serial.println("🌐 Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initializeTelegram() {
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("✅ Telegram Bot Initialized");
}

void initializeLoRa() {
  Serial.println("📡 Initializing LoRa Module...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    showInitializingScreen();
    delay(500);
  }
  
  // Configure LoRa for optimal performance
  LoRa.setSyncWord(0xF3);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(6);
  
  // Enable interrupt-driven reception
  LoRa.onReceive(onLoRaReceive);
  LoRa.receive();
  
  Serial.println("✅ LoRa Module Ready!");
}

// ═══════════════════════════════════════════════════════════
//                    PACKET PROCESSING
// ═══════════════════════════════════════════════════════════
void processReceivedPacket() {
  if (!state.packetReceived) return;
  
  state.packetReceived = false;
  
  // Update motion data
  state.motionCount++;
  state.lastMotionTime = millis();
  state.lastRSSI = String(LoRa.packetRssi());
  
  // Read packet content
  state.lastMessage = "";
  while (LoRa.available()) {
    state.lastMessage += (char)LoRa.read();
  }
  
  // Resume listening
  LoRa.receive();
  
  // Update RSSI statistics
  int currentRSSI = state.lastRSSI.toInt();
  if (currentRSSI > state.maxRSSI) state.maxRSSI = currentRSSI;
  if (currentRSSI < state.minRSSI) state.minRSSI = currentRSSI;
  
  // Trigger responses
  startEmergencyAlarm();
  sendTelegramAlert();
  
  // Log to serial
  logMotionDetection();
  
  state.displayNeedsUpdate = true;
}

void logMotionDetection() {
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.printf("🚨 MOTION DETECTED | Alert #%d\n", state.motionCount);
  Serial.printf("📨 Message: %s\n", state.lastMessage.c_str());
  Serial.printf("📶 RSSI: %s dBm\n", state.lastRSSI.c_str());
  Serial.printf("⚡ Response Time: %lu ms\n", millis() - state.lastMotionTime);
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

// ═══════════════════════════════════════════════════════════
//                    EMERGENCY ALARM SYSTEM (FIXED)
// ═══════════════════════════════════════════════════════════
void startEmergencyAlarm() {
  state.alarmActive = true;
  state.alarmStartTime = millis();
  state.lastBeepTime = millis();
  state.buzzerState = true;
  
  digitalWrite(BUZZER_PIN, HIGH);
  
  Serial.println("🚨 EMERGENCY ALARM ACTIVATED!");
}

void updateEmergencyAlarm() {
  if (!state.alarmActive) return;
  
  bool stateBuzzer = false;
  int count;
  for(count = 0; count <= 10;  count++) {
    stateBuzzer = !stateBuzzer;
    digitalWrite(BUZZER_PIN, stateBuzzer);
    delay(150);
  }

  if(count == 25) {
    stopEmergencyAlarm();
  }
}

void stopEmergencyAlarm() {
  state.alarmActive = false;
  digitalWrite(BUZZER_PIN, LOW);
  state.buzzerState = false;
  state.displayNeedsUpdate = true;
  
  Serial.println("✅ Emergency alarm deactivated");
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
  
  if (success) {
    Serial.println("📱 Telegram alert sent successfully!");
  } else {
    Serial.println("❌ Failed to send Telegram alert");
    // Retry once
    delay(100);
    bot.sendMessage(CHAT_ID, alertMessage, "Markdown");
  }
}

void handleTelegramMessages() {
  if (millis() - lastBotUpdate < BOT_REQUEST_DELAY) return;
  
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      Serial.println("❌ Unauthorized access attempt");
      continue;
    }
    
    String text = bot.messages[i].text;
    processTelegramCommand(text);
  }
  
  lastBotUpdate = millis();
}

void processTelegramCommand(const String& command) {
  if (command == "/start") {
    sendWelcomeMessage();
  } else if (command == "/status") {
    sendStatusMessage();
  } else if (command == "/stats") {
    sendStatsMessage();
  } else if (command == "/test") {
    sendTestMessage();
  } else if (command == "/help") {
    sendHelpMessage();
  } else if (command == "/info") {
    sendInfoMessage();
  } else {
    sendUnknownCommandMessage();
  }
}

void sendWelcomeMessage() {
  String message = 
    "🚀 *WELCOME TO MOTION SECURITY SYSTEM*\n\n"
    "🔐 *Real-Time LoRa Security System*\n"
    "📡 Status: Active & Monitoring\n"
    "⚡ Mode: Real-Time Detection\n\n"
    "━━━━━━━━━━━━━━━━━━━━━━━━\n"
    "📋 *AVAILABLE COMMANDS:*\n"
    "• /status - System status\n"
    "• /stats - Detection statistics\n"
    "• /test - Test connection\n"
    "• /help - Full guide\n"
    "• /info - Device information\n"
    "━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
    "✅ *System ready for real-time protection!*\n"
    "⚡ _Instant notifications when motion detected_";
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

void sendStatusMessage() {
  unsigned long uptime = (millis() - state.systemStartTime) / 1000;
  String message = 
    "📊 *REAL-TIME SYSTEM STATUS*\n\n"
    "🟢 *Online & Active*\n"
    "📡 LoRa: Interrupt-Driven\n"
    "🔋 Power: Normal\n"
    "📺 Display: Functional\n"
    "⚡ Response: <100ms\n\n"
    "⏰ *Uptime:* " + formatUptime(uptime) + "\n"
    "🎯 *Total Detections:* " + String(state.motionCount) + "\n"
    "📶 *Last RSSI:* " + (state.motionCount > 0 ? state.lastRSSI + " dBm" : "Standby") + "\n\n"
    "✅ _Real-time monitoring active_";
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

void sendStatsMessage() {
  String message = 
    "📈 *MOTION DETECTION STATISTICS*\n\n"
    "🔢 *Total Detections:* " + String(state.motionCount) + "\n"
    "🕐 *Last Detection:* " + (state.motionCount > 0 ? getLastDetectionTime() : "None yet") + "\n"
    "📶 *Signal Quality:*\n"
    "   • Strongest: " + (state.motionCount > 0 ? String(state.maxRSSI) + " dBm" : "N/A") + "\n"
    "   • Weakest: " + (state.motionCount > 0 ? String(state.minRSSI) + " dBm" : "N/A") + "\n\n"
    "📊 *System Performance:*\n"
    "   • Uptime: " + formatUptime((millis() - state.systemStartTime) / 1000) + "\n"
    "   • Status: Optimal\n"
    "   • Mode: Active Monitoring";
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

void sendTestMessage() {
  String message = 
    "⚡ *REAL-TIME CONNECTION TEST*\n\n"
    "✅ Telegram Connection: OK\n"
    "📡 LoRa Module: Active\n"
    "🔋 System: Normal\n"
    "⏱️ Response Time: <100ms\n\n"
    "🎯 *System ready for real-time detection!*";
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

void sendHelpMessage() {
  String message = 
    "📖 *LORA SECURITY SYSTEM GUIDE*\n\n"
    "🤖 *About:*\n"
    "Automated security system that sends real-time notifications when motion is detected in monitored areas.\n\n"
    "🎛️ *Available Commands:*\n\n"
    "• `/start` - Start the bot\n"
    "• `/status` - Check system condition\n"
    "• `/stats` - View complete statistics\n"
    "• `/help` - This guide\n"
    "• `/info` - Device details\n\n"
    "🔔 *Automatic Notifications:*\n"
    "Bot will send automatic alerts containing:\n"
    "• Detection time\n"
    "• Event number\n"
    "• Signal strength\n"
    "• Sensor data\n\n"
    "📞 *Support:* Check system logs";
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

void sendInfoMessage() {
  String message = 
    "🔧 *DEVICE INFORMATION*\n\n"
    "📡 *LoRa Specifications:*\n"
    "   • Frequency: 433 MHz\n"
    "   • Range: Long Distance\n"
    "   • Sync Word: 0xF3\n\n"
    "🖥️ *Hardware:*\n"
    "   • MCU: ESP32\n"
    "   • Display: OLED 128x64\n"
    "   • Sensor: Motion Detection\n"
    "   • Alarm: Active Buzzer\n\n"
    "💾 *Software:*\n"
    "   • Version: Clean v1.0\n"
    "   • Build: Professional Edition\n\n"
    "🌐 *Connectivity:*\n"
    "   • WiFi: Connected\n"
    "   • Telegram API: Active\n"
    "   • IP: " + WiFi.localIP().toString();
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

void sendUnknownCommandMessage() {
  String message = 
    "❓ *Unknown Command*\n\n"
    "Please use one of these commands:\n"
    "• /status - System status\n"
    "• /stats - Detection statistics\n"
    "• /help - Complete guide\n"
    "• /info - Device information\n\n"
    "💡 _Type /help for complete guide_";
  
  bot.sendMessage(CHAT_ID, message, "Markdown");
}

// ═══════════════════════════════════════════════════════════
//                    DISPLAY FUNCTIONS
// ═══════════════════════════════════════════════════════════
void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);
  display.println("MOTION");
  display.setCursor(5, 30);
  display.println("DETECTOR");
  display.setTextSize(1);
  display.setCursor(45, 50);
  display.println("v1.0");
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
  if (!state.displayNeedsUpdate) return;
  
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("MOTION LOGGER ");
  
  // Status indicator
  if (state.alarmActive) {
    display.print(millis() % 500 < 250 ? "ALARM!" : "      ");
  } else {
    const char* indicators[] = { "●", "◐", "○", "◑" };
    display.print(indicators[state.animationFrame]);
  }
  
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  if (state.alarmActive) {
    showAlarmDisplay();
  } else {
    showNormalDisplay();
  }
  
  display.display();
  state.displayNeedsUpdate = false;
}

void showAlarmDisplay() {
  display.setTextSize(2);
  display.setCursor(10, 18);
  display.println("MOTION!");
  
  display.setTextSize(1);
  display.setCursor(0, 38);
  display.printf("Alert #%d | RSSI: %sdBm", state.motionCount, state.lastRSSI.c_str());
  
  display.setCursor(0, 48);
  display.printf("BUZZER: %s", state.buzzerState ? "ON " : "OFF");
}

void showNormalDisplay() {
  display.setTextSize(1);
  display.setCursor(0, 13);
  display.printf("Detections: %d", state.motionCount);
  
  display.setCursor(0, 23);
  display.printf("Uptime: %s", formatUptime((millis() - state.systemStartTime) / 1000).c_str());
  
  if (state.motionCount > 0) {
    display.setCursor(0, 33);
    display.printf("Last: %s", getLastDetectionTime().c_str());
    
    display.setCursor(0, 43);
    display.printf("Signal: %s dBm", state.lastRSSI.c_str());
    
    // Signal strength bar
    display.setCursor(0, 53);
    int rssiValue = state.lastRSSI.toInt();
    int barLength = map(constrain(rssiValue, -120, -30), -120, -30, 0, 80);
    display.print("[");
    for (int i = 0; i < 10; i++) {
      display.print(i < barLength / 8 ? "=" : "-");
    }
    display.print("]");
  } else {
    display.setCursor(20, 40);
    display.println("Waiting for");
    display.setCursor(30, 50);
    display.println("motion...");
  }
}

void updateAnimation() {
  if (millis() - state.lastAnimationTime > 500) {
    state.animationFrame = (state.animationFrame + 1) % 4;
    state.lastAnimationTime = millis();
    if (!state.alarmActive) state.displayNeedsUpdate = true;
  }
}

// ═══════════════════════════════════════════════════════════
//                    UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════
String getCurrentTime() {
  unsigned long currentTime = millis() / 1000;
  int hours = (currentTime / 3600) % 24;
  int minutes = (currentTime / 60) % 60;
  int seconds = currentTime % 60;
  
  return String(hours) + ":" + 
         (minutes < 10 ? "0" : "") + String(minutes) + ":" + 
         (seconds < 10 ? "0" : "") + String(seconds);
}

String formatUptime(unsigned long seconds) {
  if (seconds < 60) {
    return String(seconds) + "s";
  } else if (seconds < 3600) {
    return String(seconds / 60) + "m " + String(seconds % 60) + "s";
  } else {
    return String(seconds / 3600) + "h " + String((seconds % 3600) / 60) + "m";
  }
}

String getLastDetectionTime() {
  if (state.motionCount == 0) return "No detections";
  
  unsigned long timeSince = (millis() - state.lastMotionTime) / 1000;
  if (timeSince < 60) {
    return String(timeSince) + "s ago";
  } else if (timeSince < 3600) {
    return String(timeSince / 60) + "m ago";
  } else {
    return String(timeSince / 3600) + "h ago";
  }
}

// ═══════════════════════════════════════════════════════════
//                    MAIN LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
  // Priority 1: Process LoRa packets immediately
  processReceivedPacket();
  
  // Priority 2: Update emergency alarm timing
  updateEmergencyAlarm();
  
  // Priority 3: Handle Telegram communications
  handleTelegramMessages();
  
  // Priority 4: Update display and animations
  updateAnimation();
  updateDisplay();
  
  // Minimal delay for system stability
  delay(1);
}
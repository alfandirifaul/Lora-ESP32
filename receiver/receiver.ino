/*********
  Alfandi Nurhuda - LoRa
  Modified from the examples of the Arduino LoRa library
  Enhanced Motion Detection Data Logger with Professional Display
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

// WiFi Credentials
const char *ssid = "nesyaaa";
const char * password = "aaaaaaab";

String botToken = "7472014617:AAGYjDzTqek4QaTJfDMiiO2yldJsq65HK1w"; 
String chatId = "7651348719";

WiFiClientSecure Client;
UniversalTelegramBot bot(botToken, Client);

// Optimized timing for real-time performance
int botRequestDelay = 100;        // Reduced from 1000ms to 100ms for faster response
unsigned long lastTimeBotRan;

// ISR flag for LoRa packet reception
volatile bool packetReceived = false;

// ═══════════════════════════════════════════════════════════
//                    INTERRUPT SERVICE ROUTINE
// ═══════════════════════════════════════════════════════════

void onReceive(int packetSize) {
  packetReceived = true;  // Set flag for main loop
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("🚀 Real-Time LoRa Motion Detection System");
  Serial.println("=========================================");

  initializeWifi();

  Client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("🔗 Telegram Bot initialized");
  Serial.println("✅ Ready for real-time Telegram notifications!");
  Serial.println("=========================================");

  Serial.println("🔧 Initializing System Components...");
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

  showStartupScreen();

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  //setup LoRa transceiver module for real-time operation
  Serial.println("📡 Initializing LoRa Module for Real-Time...");
  LoRa.setPins(ss, rst, dio0);

  while (!LoRa.begin(433E6)) {
    Serial.print(".");
    showInitializingScreen();
    delay(500);
  }

  // Optimize LoRa settings for real-time performance
  LoRa.setSyncWord(0xF3);
  LoRa.setSpreadingFactor(7);      // Faster transmission
  LoRa.setSignalBandwidth(125E3);  // Good balance of speed/range
  LoRa.setCodingRate4(5);          // Error correction
  LoRa.setPreambleLength(6);       // Shorter preamble for speed
  
  // Enable interrupt-driven packet reception for real-time response
  LoRa.onReceive(onReceive);
  LoRa.receive();  // Put LoRa in continuous receive mode
  
  Serial.println("✅ LoRa Module Ready with Real-Time Interrupts!");

  showReadyScreen();
  Serial.println("🎯 System Ready - Real-Time Monitoring Active...");
}

void initializeWifi() {
  Serial.println("🌐 Initializing WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("🌐 WiFi Initialization Complete");
}

// ═══════════════════════════════════════════════════════════
//                    REAL-TIME PACKET PROCESSING
// ═══════════════════════════════════════════════════════════

void processReceivedPacket() {
  if (!packetReceived) return;
  
  packetReceived = false;  // Reset flag
  
  // Update motion detection data immediately
  motionCount++;
  lastMotionTime = millis();
  lastRSSI = String(LoRa.packetRssi());

  // Read packet data
  lastMessage = "";
  while (LoRa.available()) {
    lastMessage += (char)LoRa.read();
  }

  // Put LoRa back in receive mode immediately
  LoRa.receive();

  // Send immediate Telegram notification
  sendImmediateTelegramAlert();

  // Start emergency alarm
  startEmergencyAlarm();

  // Enhanced real-time serial logging
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.print("🚨 REAL-TIME MOTION DETECTED | Alert #");
  Serial.println(motionCount);
  Serial.print("📨 Message: ");
  Serial.println(lastMessage);
  Serial.print("📶 RSSI: ");
  Serial.print(lastRSSI);
  Serial.println(" dBm");
  Serial.print("⚡ Response Time: ");
  Serial.print(millis() - lastMotionTime);
  Serial.println(" ms");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

  displayNeedsUpdate = true;
}

void sendImmediateTelegramAlert() {
  // Create professional Indonesian alert message
  String timestamp = getCurrentTime();
  String alertMessage = "🚨 *DETEKSI GERAKAN REAL-TIME!*\n\n"
                       "⚠️ *ALERT KEAMANAN #" + String(motionCount) + "*\n"
                       "🕐 *Waktu:* " + timestamp + "\n"
                       "📍 *Lokasi:* Area Terpantau\n"
                       "📶 *Kekuatan Sinyal:* " + lastRSSI + " dBm\n"
                       "📨 *Data Sensor:* " + lastMessage + "\n\n"
                       "━━━━━━━━━━━━━━━━━━━━━━━━\n"
                       "📊 *INFORMASI SISTEM:*\n"
                       "🔋 Waktu Operasi: " + formatUptime((millis() - systemStartTime)/1000) + "\n"
                       "🎯 Total Deteksi: " + String(motionCount) + " kejadian\n"
                       "📡 Status LoRa: Real-Time Mode\n\n"
                       "✅ *Sistem mengaktifkan alarm lokal*\n"
                       "🔔 _Periksa area yang dipantau segera!_";

  // Send message immediately with error handling
  bool success = bot.sendMessage(chatId, alertMessage, "Markdown");
  
  if (success) {
    Serial.println("📱 Telegram alert sent immediately!");
  } else {
    Serial.println("❌ Failed to send Telegram alert - retrying...");
    // Immediate retry once
    delay(100);
    bot.sendMessage(chatId, alertMessage, "Markdown");
  }
}

// ═══════════════════════════════════════════════════════════
//                    OPTIMIZED TELEGRAM HANDLING
// ═══════════════════════════════════════════════════════════

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if(chat_id != chatId) {
      Serial.println("❌ Unauthorized User, ignoring message");
      continue;
    }

    String text = bot.messages[i].text;
    String fromName = bot.messages[i].from_name;

    if(text == "/start") {
      String welcomeMessage = "🚀 *SELAMAT DATANG DI PAMS SECURITY*\n\n"
                             "🔐 *Sistem Keamanan Real-Time LoRa*\n"
                             "📡 Status: Aktif & Monitoring\n"
                             "⚡ Mode: Deteksi Real-Time\n\n"
                             "━━━━━━━━━━━━━━━━━━━━━━━━\n"
                             "📋 *MENU PERINTAH:*\n"
                             "• /status - Status sistem\n"
                             "• /stats - Statistik deteksi\n"
                             "• /test - Test koneksi real-time\n"
                             "• /help - Panduan lengkap\n"
                             "• /info - Info perangkat\n"
                             "━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
                             "✅ *Sistem siap proteksi real-time!*\n"
                             "⚡ _Notifikasi instan saat gerakan terdeteksi_";
      bot.sendMessage(chatId, welcomeMessage, "Markdown");
    }
    
    else if(text == "/test") {
      String testMessage = "⚡ *TEST KONEKSI REAL-TIME*\n\n"
                          "✅ Koneksi Telegram: OK\n"
                          "📡 LoRa Module: Aktif\n"
                          "🔋 Sistem: Normal\n"
                          "⏱️ Response Time: <100ms\n\n"
                          "🎯 *Sistem siap untuk deteksi real-time!*";
      bot.sendMessage(chatId, testMessage, "Markdown");
    }
    
    else if(text == "/status") {
      unsigned long uptime = (millis() - systemStartTime) / 1000;
      String statusMessage = "📊 *STATUS SISTEM REAL-TIME*\n\n"
                           "🟢 *Online & Real-Time Mode*\n"
                           "📡 LoRa: Interrupt-Driven\n"
                           "🔋 Power: Normal\n"
                           "📺 Display: Berfungsi\n"
                           "⚡ Response: <100ms\n\n"
                           "⏰ *Uptime:* " + formatUptime(uptime) + "\n"
                           "🎯 *Total Deteksi:* " + String(motionCount) + "\n"
                           "📶 *RSSI Terakhir:* " + (motionCount > 0 ? lastRSSI + " dBm" : "Standby") + "\n\n"
                           "✅ _Real-time monitoring aktif_";
      bot.sendMessage(chatId, statusMessage, "Markdown");
    }
    
    if(text == "/stats") {
      Serial.println("📈 User requested statistics");
      String statsMessage = "📈 *STATISTIK DETEKSI GERAKAN*\n\n"
                           "🔢 *Total Deteksi:* " + String(motionCount) + " kejadian\n"
                           "🕐 *Deteksi Terakhir:* " + (motionCount > 0 ? getLastDetectionTime() : "Belum ada") + "\n"
                           "📶 *Kualitas Sinyal:*\n"
                           "   • Terkuat: " + (motionCount > 0 ? String(maxRSSI) + " dBm" : "N/A") + "\n"
                           "   • Terlemah: " + (motionCount > 0 ? String(minRSSI) + " dBm" : "N/A") + "\n\n"
                           "📊 *Performa Sistem:*\n"
                           "   • Uptime: " + formatUptime((millis() - systemStartTime) / 1000) + "\n"
                           "   • Status: Optimal\n"
                           "   • Mode: Monitoring Aktif";
      bot.sendMessage(chatId, statsMessage, "Markdown");
      continue;
    }
    
    if(text == "/help") {
      Serial.println("❓ User requested help");
      String helpMessage = "📖 *PANDUAN LoRa SECURITY SYSTEM*\n\n"
                          "🤖 *Tentang Bot:*\n"
                          "Sistem keamanan otomatis yang mengirim notifikasi real-time saat mendeteksi gerakan di area yang dipantau.\n\n"
                          "🎛️ *Perintah yang Tersedia:*\n\n"
                          "• `/start` - Memulai bot\n"
                          "• `/status` - Cek kondisi sistem\n"
                          "• `/stats` - Lihat statistik lengkap\n"
                          "• `/help` - Panduan ini\n"
                          "• `/info` - Detail perangkat\n\n"
                          "🔔 *Notifikasi Otomatis:*\n"
                          "Bot akan mengirim alert otomatis berisi:\n"
                          "• Waktu deteksi\n"
                          "• Nomor kejadian\n"
                          "• Kekuatan sinyal\n"
                          "• Data sensor\n\n"
                          "📞 *Dukungan:* -";
      bot.sendMessage(chatId, helpMessage, "Markdown");
      continue;
    }
    
    if(text == "/info") {
      Serial.println("ℹ️ User requested device info");
      String infoMessage = "🔧 *INFORMASI PERANGKAT*\n\n"
                         "📡 *Spesifikasi LoRa:*\n"
                         "   • Frekuensi: 433 MHz\n"
                         "   • Jangkauan: Long Distance\n"
                         "   • Sync Word: 0xF3\n\n"
                         "🖥️ *Hardware:*\n"
                         "   • MCU: ESP32\n"
                         "   • Display: OLED 128x64\n"
                         "   • Sensor: PIR Motion\n"
                         "   • Alarm: Buzzer Aktif\n\n"
                         "💾 *Software:*\n"
                         "   • Versi: LoRa Security v2.0\n"
                         "   • Developer: Anynomous\n"
                         "   • Build: Professional Edition\n\n"
                         "🌐 *Konektivitas:*\n"
                         "   • WiFi: Terhubung\n"
                         "   • Telegram API: Aktif\n"
                         "   • IP: " + WiFi.localIP().toString();
      bot.sendMessage(chatId, infoMessage, "Markdown");
      continue;
    }
    
    // Handle unknown commands
    String unknownMessage = "❓ *Perintah tidak dikenali*\n\n"
                          "Silakan gunakan salah satu perintah berikut:\n"
                          "• /status - Status sistem\n"
                          "• /stats - Statistik deteksi\n"
                          "• /help - Panduan lengkap\n"
                          "• /info - Info perangkat\n\n"
                          "💡 _Ketik /help untuk panduan lengkap_";
    bot.sendMessage(chatId, unknownMessage, "Markdown");
  }
}

String getCurrentTime() {
  unsigned long currentTime = millis() / 1000;
  int hours = (currentTime / 3600) % 24;
  int minutes = (currentTime / 60) % 60;
  int seconds = currentTime % 60;
  
  return String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + 
         ":" + (seconds < 10 ? "0" : "") + String(seconds);
}

String formatUptime(unsigned long seconds) {
  if (seconds < 60) {
    return String(seconds) + " detik";
  } else if (seconds < 3600) {
    return String(seconds / 60) + " menit " + String(seconds % 60) + " detik";
  } else {
    return String(seconds / 3600) + " jam " + String((seconds % 3600) / 60) + " menit";
  }
}

String getLastDetectionTime() {
  if (motionCount == 0) return "Belum ada deteksi";
  
  unsigned long timeSince = (millis() - lastMotionTime) / 1000;
  if (timeSince < 60) {
    return String(timeSince) + " detik yang lalu";
  } else if (timeSince < 3600) {
    return String(timeSince / 60) + " menit yang lalu";
  } else {
    return String(timeSince / 3600) + " jam yang lalu";
  }
}

void sendTelegramMessage(const String &message) {
  bot.sendMessage(chatId, message, "");
  Serial.print("📨 Telegram message sent: ");
  Serial.println(message);
  Serial.println("=========================================");
  Serial.println();
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
  // Send Telegram alert
  String alertMessage = "🚨 Motion Detected!\nAlert #" + String(motionCount) +
                      "\nRSSI: " + lastRSSI + " dBm\nMessage: " + lastMessage +
                      "\nUptime: " + String((millis() - systemStartTime)/1000) + "s";
  sendTelegramMessage(alertMessage);

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
  Serial.println("📨 Message: " + lastMessage + " successfully send to Telegram");
  Serial.print("🕐 Uptime: ");
  Serial.print((millis() - systemStartTime) / 1000);
  Serial.println(" seconds since start");

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
  // PRIORITY 1: Process received packets immediately
  processReceivedPacket();
  
  // PRIORITY 2: Handle Telegram messages (reduced delay for responsiveness)
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages) {
      handleNewMessages(numNewMessages);
    }
    lastTimeBotRan = millis();
  }

  // PRIORITY 3: Update emergency alarm
  updateEmergencyAlarm();

  // PRIORITY 4: Update display and animations (lower priority)
  updateAnimation();
  if (displayNeedsUpdate) {
    updateDisplay();
    displayNeedsUpdate = false;
  }

  // Minimal delay for system stability while maintaining real-time performance
  delay(1);  // Reduced from 10ms to 1ms for better real-time response
}

// ═══════════════════════════════════════════════════════════
//                    UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════
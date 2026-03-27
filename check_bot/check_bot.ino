#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// 🔐 Replace with your details
const char* ssid = "Prem bsnl 2g";
const char* password = "prem@2025";

#define BOT_TOKEN "8676205907:AAEUIGXnacfjSNlCTeHPUboI2_8woVeKQn4"
#define CHAT_ID "6548287695"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  client.setInsecure();  // Important for ESP32

  // ✅ Send message
  bot.sendMessage(CHAT_ID, "🚽 Smart Toilet Online!", "");
}

void loop() {
}
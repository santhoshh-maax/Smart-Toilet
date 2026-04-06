#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// --- CONFIGURATION ---
const char* ssid = "ss";
const char* password = "sara225__";
#define BOT_TOKEN "8676205907:AAEUIGXnacfjSNlCTeHPUboI2_8woVeKQn4"
#define CHAT_ID "6548287695"

// --- PINS ---
#define PIR_PIN 14
#define TRIG_PIN 12
#define ECHO_PIN 13
#define DHT_PIN 27
#define MQ2_PIN 34
#define RELAY_FLUSH 26
#define RELAY_HUMID 25
#define TOILET_LIGHT 18
#define BUZZER 32
#define COMPLAINT_BUTTON 33

// --- THRESHOLDS ---
#define DIST_THRESHOLD 15
#define HUMIDITY_THRESHOLD 50
#define GAS_THRESHOLD 800
#define FLUSH_DURATION 2500 

// --- OBJECTS ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT11);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// --- STATE & STABILITY VARIABLES ---
bool isOccupied = false;
bool autoMode = true;
bool wifiConnected = false;
bool lastWifiState = false; 
int userCount = 0;
int complaintCount = 0;
float lastValidHum = 0.0;
String currentLcdMsg = "STATUS: FREE";
String lastLcdMsg = "";

// Lockout to prevent relay noise from re-triggering sensors
unsigned long lastRelayAction = 0;
const int lockoutDuration = 1500; 

// Timers
unsigned long lastTelegramCheck = 0;
unsigned long lastSensorRead = 0;
unsigned long lastFlushStart = 0;
bool flushingByTimer = false; 

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(RELAY_FLUSH, OUTPUT);
  pinMode(RELAY_HUMID, OUTPUT);
  pinMode(TOILET_LIGHT, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(COMPLAINT_BUTTON, INPUT_PULLUP);

  digitalWrite(RELAY_FLUSH, HIGH);
  digitalWrite(RELAY_HUMID, HIGH);
  digitalWrite(TOILET_LIGHT, LOW);

  lcd.begin();
  lcd.backlight();
  lcd.print("SYSTEM STARTING");
  
  WiFi.begin(ssid, password);
  client.setInsecure();
  dht.begin();
  
  delay(2000); 
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // 1. WiFi Connection Monitor
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected != lastWifiState) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (wifiConnected) {
      lcd.print("WiFi Connected");
      lcd.setCursor(0, 1);
      lcd.print("Online Mode");
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Bot Online", "");
    } else {
      lcd.print("WiFi Lost!");
      lcd.setCursor(0, 1);
      lcd.print("Offline Mode");
    }
    lastWifiState = wifiConnected;
    delay(2000); 
    lcd.clear();
    lastLcdMsg = ""; 
  }

  // 2. Telegram Control
  if (now - lastTelegramCheck > 3000) {
    if (wifiConnected) handleTelegram();
    lastTelegramCheck = now;
  }

  // 3. Sensor Logic with Noise Protection
  if (now - lastSensorRead > 300) {
    int pir = digitalRead(PIR_PIN);
    long distance = getStableDistance(); 
    float hum = dht.readHumidity();
    if (!isnan(hum)) lastValidHum = hum;
    int gas = analogRead(MQ2_PIN);

    if (autoMode && (now - lastRelayAction > lockoutDuration)) {
      if (!isOccupied && pir == HIGH && distance < DIST_THRESHOLD && distance > 1) {
        isOccupied = true;
        digitalWrite(TOILET_LIGHT, HIGH);
        triggerAutoFlush();
        currentLcdMsg = "STATUS: IN USE";
        lastRelayAction = millis();
      }
      else if (isOccupied && (distance > DIST_THRESHOLD || distance == 0)) {
        isOccupied = false;
        userCount++;
        triggerAutoFlush();
        digitalWrite(TOILET_LIGHT, LOW);
        currentLcdMsg = "STATUS: FREE";
        lastRelayAction = millis();
      }

      digitalWrite(RELAY_HUMID, (lastValidHum > HUMIDITY_THRESHOLD) ? LOW : HIGH);

      if (gas > GAS_THRESHOLD) {
        currentLcdMsg = "SMOKE ALERT!";
        digitalWrite(BUZZER, HIGH);
        if (wifiConnected) bot.sendMessage(CHAT_ID, "Smoke Alert!", "");
      } else {
        digitalWrite(BUZZER, LOW);
      }
    }
    lastSensorRead = now;
  }

  handleFlushTimer(); 
  updateLCD();
}

long getStableDistance() {
  long sum = 0;
  for(int i=0; i<3; i++) {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH, 20000);
    if (dur == 0) sum += 999;
    else sum += (dur * 0.034 / 2);
    delay(10);
  }
  return sum / 3;
}

void triggerAutoFlush() {
  digitalWrite(RELAY_FLUSH, LOW);
  lastFlushStart = millis();
  flushingByTimer = true;
}

void handleFlushTimer() {
  if (flushingByTimer && (millis() - lastFlushStart > FLUSH_DURATION)) {
    digitalWrite(RELAY_FLUSH, HIGH);
    flushingByTimer = false;
    lastRelayAction = millis(); 
  }
}

void updateLCD() {
  if (currentLcdMsg != lastLcdMsg) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(currentLcdMsg);
    lastLcdMsg = currentLcdMsg;
  }
}

// --- YOUR SPECIFIC TELEGRAM FUNCTION ---
void handleTelegram() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if (numNewMessages > 5) numNewMessages = 5;

  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    if (String(bot.messages[i].chat_id) != CHAT_ID) continue;

    if (text == "/start") {
      String welcome = "Smart Toilet Menu:\n";
      welcome += "/status - Sensors\n/auto - Reset to Auto\n";
      welcome += "/flush_on /flush_off\n/light_on /light_off\n";
      welcome += "/humid_on /humid_off\n/complaints - View Total";
      if (wifiConnected) bot.sendMessage(CHAT_ID, welcome, "");
    }
    else if (text == "/auto") {
      autoMode = true; 
      flushingByTimer = false;
      lastRelayAction = millis(); // Using shared lockout variable
      digitalWrite(RELAY_FLUSH, HIGH);
      if (wifiConnected) bot.sendMessage(CHAT_ID, "System reset to Auto Mode", "");
    }
    else if (text == "/status") {
      String msg = "Occupancy: " + String(isOccupied ? "BUSY" : "FREE") + "\n";
      msg += "Users: " + String(userCount) + "\nComplaints: " + String(complaintCount) + "\nHumidity: " + String(lastValidHum) + "%";
      if (wifiConnected) bot.sendMessage(CHAT_ID, msg, "");
    }
    else if (text == "/flush_on") {
      autoMode = false; flushingByTimer = false;
      digitalWrite(RELAY_FLUSH, LOW);
      lastRelayAction = millis();
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Manual Flush ON", "");
    }
    else if (text == "/flush_off") {
      digitalWrite(RELAY_FLUSH, HIGH);
      lastRelayAction = millis();
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Manual Flush OFF", "");
    }
    else if (text == "/humid_on") {
      autoMode = false; digitalWrite(RELAY_HUMID, LOW);
      lastRelayAction = millis();
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Humidifier ON", "");
    }
    else if (text == "/humid_off") {
      autoMode = false; digitalWrite(RELAY_HUMID, HIGH);
      lastRelayAction = millis();
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Humidifier OFF", "");
    }
    else if (text == "/light_on") {
      autoMode = false; digitalWrite(TOILET_LIGHT, HIGH);
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Light ON", "");
    }
    else if (text == "/light_off") {
      autoMode = false; digitalWrite(TOILET_LIGHT, LOW);
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Light OFF", "");
    }
    else if (text == "/complaints") {
      if (wifiConnected) bot.sendMessage(CHAT_ID, "Total Complaints: " + String(complaintCount), "");
    }
  }
}
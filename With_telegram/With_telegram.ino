#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
// ---------------- PINS ----------------
#define PIR_PIN 14
#define TRIG_PIN 12
#define ECHO_PIN 13
#define DHT_PIN 27
#define MQ2_PIN 34
#define RELAY_FLUSH 26  // orange color relay IN wire
#define RELAY_HUMID 25 // Yellow color relay IN wire
#define TOILET_LIGHT 18
#define BUZZER 32
#define COMPLAINT_BUTTON 5
// ---------------- SENSORS ----------------
DHT dht(DHT_PIN, DHT11);
// ---------------- VARIABLES ----------------
bool isOccupied = false;
int userCount = 0;
int complaintCount = 0;
#define DIST_THRESHOLD 25
#define HUMIDITY_THRESHOLD 50
#define GAS_THRESHOLD 800
//------------- Wifi for Telegram ----------
const char* ssid = "ss";
const char* password = "sara225__";
#define BOT_TOKEN "8676205907:AAEUIGXnacfjSNlCTeHPUboI2_8woVeKQn4"
#define CHAT_ID "6548287695"
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
unsigned long lastMotionTime = 0;
const unsigned long lightTimeout = 10000;
unsigned long lastModeChangeTime = 0;
// Debug
unsigned long lastDebugTime = 0;
const unsigned long debugInterval = 2000;
unsigned long lastStateChange = 0;
const int stateDelay = 3000; // 3 sec lock
//telegram call
unsigned long lastTelegramCheck = 0;
const int telegramInterval = 1000; // 1 second
// Button
bool lastComplaintState = HIGH;
unsigned long lastComplaintDebounce = 0;
const int debounceDelay = 150;
unsigned long lastSystemEventTime = 0;
bool firstRun = true;
// -------- TELEGRAM CONTROL --------
bool sendAutoMsg = false;
bool sendSmokeMsg = false;
bool manualHumid = false;
bool manualLight = false;
bool manualBuzzer = false;
bool manualFlush = false;
bool smokeNotified = false;
bool autoMode = true;   // 🔥 AUTO mode default
// Gas
bool gasAlert = false;
// Flush
unsigned long flushTimer = 0;
bool flushing = false;
// LCD
unsigned long lcdTimer = 0;
bool showTempMessage = false;
// Ultrasonic timing
unsigned long lastUltraTime = 0;
long distance = -1;
String lastStatus = "";
// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_FLUSH, OUTPUT);
  digitalWrite(RELAY_FLUSH, HIGH);   // 🔥 OFF
  pinMode(RELAY_HUMID, OUTPUT);
  digitalWrite(RELAY_HUMID, HIGH);   // 🔥 OFF
  pinMode(TOILET_LIGHT, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(COMPLAINT_BUTTON, INPUT_PULLUP);
   WiFi.begin(ssid, password);

// -------- LCD INIT (moved earlier) --------
lcd.begin();
lcd.backlight();

// -------- SHOW CONNECTING --------
lcd.setCursor(0, 0);
lcd.print("Connecting WiFi");

Serial.print("Connecting");

int dotCount = 0;

while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");

  // LCD dots animation
  lcd.setCursor(dotCount, 1);
  lcd.print(".");
  dotCount++;

  if (dotCount > 15) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    dotCount = 0;
  }
}

// -------- WIFI CONNECTED --------
Serial.println("\nWiFi Connected");

lcd.clear();
lcd.setCursor(0, 0);
lcd.print("WiFi Connected");
lcd.setCursor(0, 1);
lcd.print("Ready...");
delay(2000);
lcd.clear();

// -------- TELEGRAM --------
client.setInsecure();
client.setTimeout(1000);   // 1 second max wait
bot.sendMessage(CHAT_ID, "🚽 Smart Toilet Online!", "");

// -------- SENSOR --------
dht.begin();

// -------- START SCREEN --------
lcd.setCursor(0, 0);
lcd.print("Smart Toilet");
delay(2000);
lcd.clear();

delay(1000); // stabilize button
}
// ---------------- LOOP ----------------
void loop() {

  unsigned long currentMillis = millis();
    // 🔥 WIFI AUTO-RECONNECT (ADD HERE)
static unsigned long lastReconnectAttempt = 0;

if (WiFi.status() != WL_CONNECTED && millis() - lastReconnectAttempt > 5000) {
  Serial.println("WiFi lost! Reconnecting...");
  WiFi.begin(ssid, password);
  lastReconnectAttempt = millis();
}

  int pirState = digitalRead(PIR_PIN);
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
  humidity = 0;
}
  int gasValue = analogRead(MQ2_PIN);
  // -------- ULTRASONIC (STABLE) --------
  if (currentMillis - lastUltraTime > 100) {
    distance = getDistance();
    lastUltraTime = currentMillis;
  }
  // -------- DEBUG PRINT --------
  if (currentMillis - lastDebugTime > debugInterval) {
    lastDebugTime = currentMillis;
    printSystemStatus(pirState, distance, humidity, gasValue, currentMillis);
  }
  // -------- LIGHT --------
  if (pirState == HIGH) {
  lastMotionTime = currentMillis;
}
  if (autoMode && !manualLight) {
  digitalWrite(TOILET_LIGHT,
    (currentMillis - lastMotionTime < lightTimeout) ? HIGH : LOW);
}
// -------- ENTRY --------
  if (autoMode && pirState == HIGH && distance > 0 && distance < DIST_THRESHOLD && !isOccupied 
    && currentMillis - lastStateChange > stateDelay) { 
    Serial.println(">>> ENTRY DETECTED <<<");
    isOccupied = true;
    
      flushing = true;
      flushTimer = currentMillis;

    lastStateChange = currentMillis;
    lastSystemEventTime = currentMillis;
  }
  // -------- FLUSH --------
 if (autoMode && !manualFlush) {

  if (flushing) {
    if (currentMillis - flushTimer < 2000) {
      digitalWrite(RELAY_FLUSH, LOW);   // ON
    } else {
      digitalWrite(RELAY_FLUSH, HIGH);  // OFF
      flushing = false;
    }
  } else {
    digitalWrite(RELAY_FLUSH, HIGH);    // always ensure OFF
  }
}
  // -------- EXIT --------
 if (autoMode && isOccupied && distance > DIST_THRESHOLD && distance > 0 
    && currentMillis - lastStateChange > stateDelay) { 
    isOccupied = false;
    userCount++;
    Serial.println(">>> EXIT DETECTED <<<");

  flushing = true;
  flushTimer = currentMillis;

    lastStateChange = currentMillis;
    lastSystemEventTime = currentMillis;
  }
  // -------- HUMIDITY --------
  if (autoMode) {
  if (!manualHumid && humidity > HUMIDITY_THRESHOLD) {
    digitalWrite(RELAY_HUMID, LOW);
    if (!showTempMessage) displayOnce("Bad Air!");
  } else {
    digitalWrite(RELAY_HUMID, HIGH);
  }
}
  // -------- GAS --------
if (autoMode && !manualBuzzer && gasValue > GAS_THRESHOLD && !gasAlert) {

  gasAlert = true;

  digitalWrite(BUZZER, HIGH);

  if (!showTempMessage) displayOnce("SMOKE ALERT!");

  if (!smokeNotified) {
  sendSmokeMsg = true;   // 🔥 set flag only
  smokeNotified = true;
}
}

// -------- RESET --------
if (autoMode && gasValue < GAS_THRESHOLD) {

  gasAlert = false;
  digitalWrite(BUZZER, LOW);

  smokeNotified = false;
}
// -------- BUTTON --------
  checkComplaintButton(currentMillis);
if (millis() - lastTelegramCheck > telegramInterval) {

  if (WiFi.status() == WL_CONNECTED) {
    handleTelegram();
  }

  lastTelegramCheck = millis();
}

//----------TELEGRAM MESSAGE NO_BLOCKING
static unsigned long lastMsgTime = 0;
if (sendAutoMsg && millis() - lastMsgTime > 1000) {
  bot.sendMessage(CHAT_ID, "AUTO mode enabled", "");
  sendAutoMsg = false;
  lastMsgTime = millis();
}
static unsigned long lastSmokeMsgTime = 0;

if (sendSmokeMsg && millis() - lastSmokeMsgTime > 1000) {
  bot.sendMessage(CHAT_ID, "🚬 Smoke detected in toilet!", "");
  sendSmokeMsg = false;
  lastSmokeMsgTime = millis();
}
// -------- LCD RETURN --------
  String currentStatus = isOccupied ? "Status: BUSY    " : "Status: FREE    ";
if (showTempMessage) {
  if (currentMillis - lcdTimer > 3000) {
    showTempMessage = false;

    lcd.setCursor(0, 0);
    lcd.print("                ");  // clear line
    lcd.setCursor(0, 0);
    lcd.print(currentStatus);

    lastStatus = currentStatus;
  }
} else {
if (!showTempMessage && currentStatus != lastStatus) {
    lcd.setCursor(0, 0);
    lcd.print(currentStatus);
    lastStatus = currentStatus;
  }
}
}
// ---------------- FUNCTIONS ----------------
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(50);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

void displayOnce(String msg) {
  showTempMessage = true;
  lcdTimer = millis();

  lcd.setCursor(0, 0);
  lcd.print("                "); // clear line safely
  lcd.setCursor(0, 0);
  lcd.print(msg);
}

void checkComplaintButton(unsigned long currentMillis) {

  if (currentMillis - lastModeChangeTime < 1000) return;
  if (currentMillis - lastSystemEventTime < 1500) return;

  if (firstRun) {
    lastComplaintState = digitalRead(COMPLAINT_BUTTON);
    firstRun = false;
    return;
  }

  int reading = digitalRead(COMPLAINT_BUTTON);

  if (reading == LOW && lastComplaintState == HIGH) {

    if (currentMillis - lastComplaintDebounce > debounceDelay) {

      complaintCount++;
      lastComplaintDebounce = currentMillis;

      Serial.println("Complaint Registered!");
      displayOnce("Complaint Sent");

      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);

      Serial.print("Total Complaints: ");
      Serial.println(complaintCount);
    }
  }

  lastComplaintState = reading;
}
// ------------Telagram function -------------
void handleTelegram() {
  int numNewMessages = 0;

if (WiFi.status() == WL_CONNECTED) {
  numNewMessages = bot.getUpdates(bot.last_message_received + 1);
}
  while (numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
      String text = bot.messages[i].text;
      Serial.println("Received: " + text);
      // -------- START --------
      if (text == "/start") {
        bot.sendMessage(CHAT_ID,
        "/status\n/auto\n/flush_on\n/flush_off\n/humid_on\n/humid_off\n/light_on\n/light_off\n/complaints\n/buzzer_on\n/buzzer_off", "");
      }
      // -------- AUTO MODE --------
      else if (text == "/auto") {

  autoMode = true;

  manualFlush = false;
  manualHumid = false;
  manualLight = false;
  manualBuzzer = false;

  // 🔥 IMPORTANT FIX (prevents false complaint trigger)
  lastComplaintState = digitalRead(COMPLAINT_BUTTON);
  lastComplaintDebounce = millis();

  // 🔥 Optional extra safety (recommended)
  lastModeChangeTime = millis();
   sendAutoMsg = true;

}
      // -------- STATUS --------
      else if (text == "/status") {
        String msg = "🚽 Smart Toilet\n";
        msg += autoMode ? "Mode: AUTO\n" : "Mode: MANUAL\n";
        msg += (isOccupied ? "Status: BUSY\n" : "Status: FREE\n");
        msg += "Complaints: " + String(complaintCount);
        bot.sendMessage(CHAT_ID, msg, "");
      }
      // -------- FLUSH --------
      else if (text == "/flush_on") {
        autoMode = false;
        manualFlush = true;
        digitalWrite(RELAY_FLUSH, LOW);
        bot.sendMessage(CHAT_ID, "Flush ON", "");
      }
      else if (text == "/flush_off") {
        autoMode = false;
        manualFlush = false;
        digitalWrite(RELAY_FLUSH, HIGH);
        bot.sendMessage(CHAT_ID, "Flush OFF", "");
      }
      // -------- HUMIDIFIER --------
      else if (text == "/humid_on") {
        autoMode = false;
        manualHumid = true;
        digitalWrite(RELAY_HUMID, LOW);
        bot.sendMessage(CHAT_ID, "Humidifier ON", "");
      }
      else if (text == "/humid_off") {
        autoMode = false;
        manualHumid = false;
        digitalWrite(RELAY_HUMID, HIGH);
        bot.sendMessage(CHAT_ID, "Humidifier OFF", "");
      }
      // -------- LIGHT --------
      else if (text == "/light_on") {
        autoMode = false;
        manualLight = true;
        digitalWrite(TOILET_LIGHT, HIGH);
        bot.sendMessage(CHAT_ID, "Light ON", "");
      }
      else if (text == "/light_off") {
        autoMode = false;
        manualLight = false;
        digitalWrite(TOILET_LIGHT, LOW);
        bot.sendMessage(CHAT_ID, "Light OFF", "");
      }
      // -------- BUZZER --------
      else if (text == "/buzzer_on") {
        autoMode = false;
        manualBuzzer = true;
        digitalWrite(BUZZER, HIGH);
        bot.sendMessage(CHAT_ID, "Buzzer ON", "");
      }
      else if (text == "/buzzer_off") {
        autoMode = false;
        manualBuzzer = false;
        digitalWrite(BUZZER, LOW);
        bot.sendMessage(CHAT_ID, "Buzzer OFF", "");
      }
      // -------- COMPLAINT --------
      else if (text == "/complaints") {
        bot.sendMessage(CHAT_ID, "Complaints: " + String(complaintCount), "");
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}


void printSystemStatus(int pirState, long distance, float humidity, int gasValue, unsigned long currentMillis) {
  Serial.println("------ SYSTEM STATUS ------");
  Serial.print("PIR: ");
  Serial.println(pirState ? "Motion" : "No Motion");
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Gas Value: ");
  Serial.println(gasValue);
  Serial.print("Toilet: ");
  Serial.println(isOccupied ? "OCCUPIED" : "FREE");
  Serial.print("Light: ");
  Serial.println((currentMillis - lastMotionTime < lightTimeout) ? "ON" : "OFF");
  Serial.print("Flushing: ");
  Serial.println(flushing ? "YES" : "NO");
  Serial.print("Complaints: ");
  Serial.println(complaintCount);
  Serial.print("Mode: ");
  Serial.println(autoMode ? "AUTO" : "MANUAL");
  Serial.println("---------------------------\n");
}
#include <Wire.h>
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

#define RELAY_FLUSH 26
#define RELAY_HUMID 25

#define LED_GREEN 2
#define LED_YELLOW 15
#define LED_RED 33
#define TOILET_LIGHT 18

#define BUZZER 32
#define COMPLAINT_BUTTON 5

// ---------------- SENSORS ----------------
DHT dht(DHT_PIN, DHT11);

// ---------------- VARIABLES ----------------
bool isOccupied = false;
int userCount = 0;
int complaintCount = 0;

#define DIST_THRESHOLD 100
#define HUMIDITY_THRESHOLD 75
#define GAS_THRESHOLD 2000

unsigned long lastMotionTime = 0;
const unsigned long lightTimeout = 20000;

bool lastComplaintState = HIGH;
unsigned long lastComplaintDebounce = 0;
const int debounceDelay = 50;

bool gasAlert = false;

// Non-blocking timers
unsigned long flushTimer = 0;
bool flushing = false;

unsigned long lcdTimer = 0;
bool showTempMessage = false;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_FLUSH, OUTPUT);
  pinMode(RELAY_HUMID, OUTPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(TOILET_LIGHT, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(COMPLAINT_BUTTON, INPUT_PULLUP);

  dht.begin();

  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Toilet");

  setLEDState("FREE");
}

// ---------------- LOOP ----------------
void loop() {

  int pirState = digitalRead(PIR_PIN);
  long distance = getDistance();
  float humidity = dht.readHumidity();
  int gasValue = analogRead(MQ2_PIN);

  unsigned long currentMillis = millis();

  // -------- SMART LIGHT --------
  if (pirState == HIGH) {
    lastMotionTime = currentMillis;
  }

  if (currentMillis - lastMotionTime < lightTimeout) {
    digitalWrite(TOILET_LIGHT, HIGH);
  } else {
    digitalWrite(TOILET_LIGHT, LOW);
  }

  // -------- ENTRY --------
  if (pirState == HIGH && distance < DIST_THRESHOLD && !isOccupied) {
    setLEDState("ENTERING");

    isOccupied = true;
    flushing = true;
    flushTimer = currentMillis;
  }

  // -------- FLUSH CONTROL --------
  if (flushing) {
    if (currentMillis - flushTimer < 2000) {
      digitalWrite(RELAY_FLUSH, LOW);
    } else {
      digitalWrite(RELAY_FLUSH, HIGH);
      flushing = false;
      setLEDState("OCCUPIED");
    }
  }

  // -------- EXIT --------
  if (isOccupied && distance > DIST_THRESHOLD) {
    isOccupied = false;
    userCount++;

    setLEDState("FREE");

    flushing = true;
    flushTimer = currentMillis;
  }

  // -------- HUMIDITY --------
  if (humidity > HUMIDITY_THRESHOLD) {
    digitalWrite(RELAY_HUMID, LOW);
    displayOnce("Bad Air!");
  } else {
    digitalWrite(RELAY_HUMID, HIGH);
  }

  // -------- GAS --------
  if (gasValue > GAS_THRESHOLD && !gasAlert) {
    gasAlert = true;

    digitalWrite(BUZZER, HIGH);
    displayOnce("SMOKE ALERT!");
  }

  if (gasValue < GAS_THRESHOLD - 200) {
    gasAlert = false;
    digitalWrite(BUZZER, LOW);
  }

  // -------- BUTTON --------
  checkComplaintButton(currentMillis);

  // -------- LCD RETURN --------
  if (showTempMessage && currentMillis - lcdTimer > 2000) {
    showTempMessage = false;

    if (isOccupied) setLEDState("OCCUPIED");
    else setLEDState("FREE");
  }
}

// ---------------- FUNCTIONS ----------------

long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void setLEDState(String state) {

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  if (state == "FREE") {
    digitalWrite(LED_GREEN, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Status: FREE   ");
  }
  else if (state == "ENTERING") {
    digitalWrite(LED_YELLOW, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("User Detected  ");
  }
  else if (state == "OCCUPIED") {
    digitalWrite(LED_RED, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Status: BUSY   ");
  }
}

void displayOnce(String msg) {
  lcd.clear();
  lcd.print(msg);

  showTempMessage = true;
  lcdTimer = millis();
}

void checkComplaintButton(unsigned long currentMillis) {

  int reading = digitalRead(COMPLAINT_BUTTON);

  if (reading == LOW && lastComplaintState == HIGH) {

    if (currentMillis - lastComplaintDebounce > debounceDelay) {

      complaintCount++;
      lastComplaintDebounce = currentMillis;

      Serial.println("Complaint Registered!");

      digitalWrite(BUZZER, HIGH);
      displayOnce("Complaint Sent");

    }
  }

  lastComplaintState = reading;
}
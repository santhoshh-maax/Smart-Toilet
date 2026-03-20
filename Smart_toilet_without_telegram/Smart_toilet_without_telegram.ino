#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- PINS ----------------
#define PIR_PIN        14
#define TRIG_PIN       12
#define ECHO_PIN       13
#define DHT_PIN        27
#define MQ2_PIN        34

#define RELAY_FLUSH    26
#define RELAY_HUMID    25

#define LED_GREEN      2
#define LED_YELLOW     15
#define LED_RED        33

#define BUZZER         32
#define COMPLAINT_BUTTON 5

// ---------------- SENSORS ----------------
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ---------------- VARIABLES ----------------
bool isOccupied = false;
int userCount = 0;
int complaintCount = 0;

#define DIST_THRESHOLD 100
#define HUMIDITY_THRESHOLD 75
#define GAS_THRESHOLD 2000
#define CLEAN_THRESHOLD 10

unsigned long lastButtonPress = 0;
const int debounceDelay = 300;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);

  pinMode(RELAY_FLUSH, OUTPUT);
  pinMode(RELAY_HUMID, OUTPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(COMPLAINT_BUTTON, INPUT_PULLUP);

  dht.begin();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Toilet");
  delay(2000);
  lcd.clear();

  setLEDState("FREE");
}

// ---------------- LOOP ----------------
void loop() {

  int pirState = digitalRead(PIR_PIN);
  long distance = getDistance();
  float humidity = dht.readHumidity();
  int gasValue = analogRead(MQ2_PIN);

  // ENTRY
  if (pirState == HIGH && distance < DIST_THRESHOLD && !isOccupied) {

    setLEDState("ENTERING");
    delay(2000);

    if (getDistance() < DIST_THRESHOLD) {
      setLEDState("OCCUPIED");
      isOccupied = true;

      // Pre-flush
      digitalWrite(RELAY_FLUSH, HIGH);
      delay(2000);
      digitalWrite(RELAY_FLUSH, LOW);
    }
  }

  // EXIT
  if (isOccupied && distance > DIST_THRESHOLD) {

    setLEDState("FREE");
    isOccupied = false;

    // Post-flush
    digitalWrite(RELAY_FLUSH, HIGH);
    delay(3000);
    digitalWrite(RELAY_FLUSH, LOW);

    userCount++;
    Serial.println("User Count: " + String(userCount));
  }

  // HUMIDITY CONTROL
  if (humidity > HUMIDITY_THRESHOLD) {
    digitalWrite(RELAY_HUMID, HIGH);
    displayBadAir();
  } else {
    digitalWrite(RELAY_HUMID, LOW);
  }

  // SMOKE DETECTION
  if (gasValue > GAS_THRESHOLD) {
    digitalWrite(BUZZER, HIGH);
    displaySmoke();
    Serial.println("SMOKE DETECTED!");
    delay(2000);
    digitalWrite(BUZZER, LOW);
  }

  // COMPLAINT BUTTON
  checkComplaintButton();

  // MAINTENANCE ALERT
  if (userCount >= CLEAN_THRESHOLD) {
    Serial.println("Cleaning Required!");
    userCount = 0;
  }

  delay(500);
}

// ---------------- FUNCTIONS ----------------

// Ultrasonic
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

// LED + LCD
void setLEDState(String state) {

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  lcd.clear();

  if (state == "FREE") {
    digitalWrite(LED_GREEN, HIGH);
    lcd.print("Status: FREE");
  }
  else if (state == "ENTERING") {
    digitalWrite(LED_YELLOW, HIGH);
    lcd.print("User Detected");
  }
  else if (state == "OCCUPIED") {
    digitalWrite(LED_RED, HIGH);
    lcd.print("Status: BUSY");
  }
}

// LCD Messages
void displayBadAir() {
  lcd.clear();
  lcd.print("Bad Air!");
}

void displaySmoke() {
  lcd.clear();
  lcd.print("SMOKE ALERT!");
}

// Complaint Button
void checkComplaintButton() {
  if (digitalRead(COMPLAINT_BUTTON) == LOW) {
    if (millis() - lastButtonPress > debounceDelay) {

      complaintCount++;
      lastButtonPress = millis();

      Serial.println("Complaint Registered!");

      digitalWrite(BUZZER, HIGH);

      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_RED, HIGH);
        delay(300);
        digitalWrite(LED_RED, LOW);
        delay(300);
      }

      digitalWrite(BUZZER, LOW);

      displayBadAir();
    }
  }
}
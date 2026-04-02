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
#define HUMIDITY_THRESHOLD 70
#define GAS_THRESHOLD 800

unsigned long lastMotionTime = 0;
const unsigned long lightTimeout = 10000;

// Debug
unsigned long lastDebugTime = 0;
const unsigned long debugInterval = 2000;

unsigned long lastStateChange = 0;
const int stateDelay = 3000; // 3 sec lock

// Button
bool lastComplaintState = HIGH;
unsigned long lastComplaintDebounce = 0;
const int debounceDelay = 50;
bool firstRun = true;

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
  pinMode(RELAY_HUMID, OUTPUT);

  pinMode(TOILET_LIGHT, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(COMPLAINT_BUTTON, INPUT_PULLUP);

  dht.begin();

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Toilet");
  delay(2000);
  lcd.clear();

  delay(1000); // stabilize button
}

// ---------------- LOOP ----------------
void loop() {

  unsigned long currentMillis = millis();

  int pirState = digitalRead(PIR_PIN);
  float humidity = dht.readHumidity();
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

  digitalWrite(TOILET_LIGHT,
    (currentMillis - lastMotionTime < lightTimeout) ? HIGH : LOW);

  // -------- ENTRY --------
  if (pirState == HIGH && distance > 0 && distance < DIST_THRESHOLD && !isOccupied) {
    Serial.println(">>> ENTRY DETECTED <<<");

    isOccupied = true;
    flushing = true;
    flushTimer = currentMillis;

    lastStateChange = currentMillis;
  }

  // -------- FLUSH --------
  if (flushing) {
    if (currentMillis - flushTimer < 3000) {
      digitalWrite(RELAY_FLUSH, LOW);
    } else {
      digitalWrite(RELAY_FLUSH, HIGH);
      flushing = false;
    }
  }

  // -------- EXIT --------
  if (isOccupied && distance > DIST_THRESHOLD && distance > 0) {
    isOccupied = false;
    userCount++;

    Serial.println(">>> EXIT DETECTED <<<");

    flushing = true;
    flushTimer = currentMillis;

    lastStateChange = currentMillis;
  }

  // -------- HUMIDITY --------
  if (humidity > HUMIDITY_THRESHOLD) {
    digitalWrite(RELAY_HUMID, LOW);
    if (!showTempMessage) displayOnce("Bad Air!");
  } else {
    digitalWrite(RELAY_HUMID, HIGH);
  }

  // -------- GAS --------
  if (gasValue > GAS_THRESHOLD && !gasAlert) {
    gasAlert = true;

    digitalWrite(BUZZER, HIGH);
    if (!showTempMessage) displayOnce("Smoke Alert!");
  }

  if (gasValue < GAS_THRESHOLD - 200) {
    gasAlert = false;
    digitalWrite(BUZZER, LOW);
  }

  // -------- BUTTON --------
  checkComplaintButton(currentMillis);

  // -------- LCD RETURN --------
  String currentStatus = isOccupied ? "Status: BUSY    " : "Status: FREE    ";

if (showTempMessage) {

  if (currentMillis - lcdTimer > 3000) {
    showTempMessage = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(currentStatus);

    lastStatus = currentStatus;
  }

} else {

  if (currentStatus != lastStatus) {
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
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);   // ONLY message


}

void checkComplaintButton(unsigned long currentMillis) {

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
      delay(200);
      digitalWrite(BUZZER, LOW);

      

      Serial.print("Total Complaints: ");
      Serial.println(complaintCount);
    }
  }

  lastComplaintState = reading;
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

  Serial.println("---------------------------\n");
}
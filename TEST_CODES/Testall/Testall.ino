#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define PIR_PIN   14
#define TRIG_PIN  12
#define ECHO_PIN  13
#define DHT_PIN   27
#define MQ2_PIN   34

// #define RELAY_PIN  25
#define LED_PIN    2
#define COMPLAINT_BUTTON 5

DHT dht(DHT_PIN, DHT11);

bool state = false;
bool lastState = HIGH;

unsigned long lastDebounce = 0;
unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  // pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(COMPLAINT_BUTTON, INPUT_PULLUP);

  dht.begin();

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Testing LCD...");
}

void loop() {

  // PIR
  int pirState = digitalRead(PIR_PIN);
  Serial.println(pirState ? "Motion Detected" : "No Motion");

  // MQ2
  int gasValue = analogRead(MQ2_PIN);
  Serial.print("Gas: ");
  Serial.println(gasValue);

  // DHT
  float humidity = dht.readHumidity();
  Serial.print("Humidity: ");
  Serial.println(humidity);

  // Ultrasonic
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Relay test
  // digitalWrite(RELAY_PIN, LOW);
  // delay(1000);
  // digitalWrite(RELAY_PIN, HIGH);
  // delay(1000);

  // LED test
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  // Button test
  int reading = digitalRead(COMPLAINT_BUTTON);

  if(reading == LOW && lastState == HIGH){
    if((millis() - lastDebounce) > debounceDelay){

      state = !state;

      Serial.print("Button State: ");
      Serial.println(state ? "ON" : "OFF");

      lastDebounce = millis();
    }
  }

  lastState = reading;
}
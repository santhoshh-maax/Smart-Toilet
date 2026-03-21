//header file section
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

//---------LCD----------

LiquidCrystal_I2C lcd(0x27, 16, 2);

//--------pins--------
#define PIR_PIN     14
#define TRIG_PIN    12
#define ECHO_PIN    13
#define DHT_PIN     27
#define MQ2_PIN     34

#define RELAY_FLUSH  26
#define RELAY_HUMID  25

#define LED_GREEN    2
#define LED_YELLOW   15
#define LED_RED      33

#define BUZZER        32
#define COMPLAINT_BUTTON 5

//-------sensors---------

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

//--------variables------


bool isOccupied = false;
int userCount = 0;
int ComplaintCount = 0;

#define DIST_THERSHOLD 25
#define HUMIDITY_THERSHOLD 75
#define GAS_THERSHOLD 800
#define CLEAN_THERSHOLD 20

bool lastComplaintState = HIGH;
unsigned long lastComplaintDebounce = 0;
const int debounceDelay = 50;

//----------setup---------------

void setup(){
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

  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Toilet");
  delay(2000);
  lcd.clear();

  setLEDState("FREE");
}

void loop(){
  int pirState = digitalWrite(PIR_PIN);
  long distance = getDistance();
  float humitidy = dht.readHumidity();
  int gasValue = analogRead(MQ2_PIN);

  //Entery

  if(pirState == HIGH && distance < DIST_THERSHOLD && !isOccupied){
      setLEDState("ENTERING");
      delay(2000);

      if(getDistance() < DIST_THERSHOLD){
        setLEDState("OCCUPIED");
        isOccupied = true;

        //pre-flush
        digitalWrite(RELAY_FLUSH, LOW);
        delay(2000);
        digitalWrite(RELAY_FLUSH, HIGH);
      }
  }

  //EXIT
  if(isOccupied && distance > DIST_THERSHOLD){
    SetLEDState("FREE");
    isOccupied = false;

    digitalWrite(RELAY_FLUSH, LOW);
    delay(2000);
    digitalWrite(RELAY_FLUSH, HIGH);

    userCount++;
    Serial.println("User Count: " + String(userCount));


  }

  //humidity

  if(humitidy > HUMIDITY_THERSHOLD){
    digitalWrite(RELAY_HUMID, LOW);
    displayBadAir();
    }
    else{
    digitalWrite(RELAY_HUMID, HIGH);
    }
    
  //smoke detection

  if(gasValue > GAS_THERSHOLD){
    digitalWrite(BUZZER, HIGH);
    displaySmoke();
    Serial.println("SMOKE DETECTED!!");
    delay(2000);
    digitalWrite(BUZZER, LOW);
  }

  //complaint button
  checkComplaintButton();

  //maintaince alert

  if(userCount > CLEAN_THERSHOLD){
    Serial.println("Cleaning Required");
    userCount = 0;
  }
  delay(500);

}

//-----functions-------

//ultrasonic

long getDistance(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ECHO_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

// LED + LCD
void setLEDState(String state){
  
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  lcd.clear();

  if(state == "FREE"){
    digitalWrite(LED_GREEN, HIGH);
    lcd.print("Status: FREE");
  }

  else if(state == "ENTERING"){
    digitalWrite(LED_YELLOW, HIGH);
    lcd.print("User Detected");
  }

  else if(state == "OCCUPIED"){
    digitalWrite(LED_RED, HIGH);
    lcd.print("Status: BUSY");

  }

}

//LCD message

void displayBadAir(){
  lcd.clear();
  lcd.print("Bad Air!");
}

void displaySmoke(){
  lcd.clear();
  lcd.print("SMOKE ALERT!");
}

//Complaint Button



void checkComplaintButton() {

  int reading = digitalRead(COMPLAINT_BUTTON);

  // Detect button press (falling edge)
  if (reading == LOW && lastComplaintState == HIGH) {

    if ((millis() - lastComplaintDebounce) > debounceDelay) {

      ComplaintCount++;
      lastComplaintDebounce = millis();

      Serial.println("Complaint Registered!");
      Serial.print("Total Complaints: ");
      Serial.println(complaintCount);

      //  Buzzer + LED indication
      digitalWrite(BUZZER, HIGH);

      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_RED, HIGH);
        delay(200);
        digitalWrite(LED_RED, LOW);
        delay(200);
      }

      digitalWrite(BUZZER, LOW);

      //  LCD DISPLAY (IMPORTANT)
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Complaint Sent");
      lcd.setCursor(0, 1);
      lcd.print("Total: ");
      lcd.print(complaintCount);

      delay(2000);  // show message

      // Return to normal screen
      if (isOccupied) {
        setLEDState("OCCUPIED");
      } else {
        setLEDState("FREE");
      }
    }
  }

  lastComplaintState = reading;
}
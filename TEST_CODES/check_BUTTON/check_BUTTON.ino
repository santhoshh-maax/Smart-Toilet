#define BUTTON 13

bool state = false;
bool lastState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup(){
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop(){
  int reading = digitalRead(BUTTON);

  if(reading == LOW && lastState == HIGH){
    if((millis() - lastDebounceTime) > debounceDelay){
      state = !state;

      Serial.print("State: ");
      Serial.println(state ? "ON" : "OFF");

      lastDebounceTime = millis();
    }
  }
  lastState = reading;
}
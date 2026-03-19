#define BUTTON 4

bool state = false;
bool lastState = HIGH; // for input_pull

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup(){
  Serial.begin(9600);
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
#define PIR 14
void setup() {
  Serial.begin(9600);
  pinMode(PIR, INPUT);
  

}

void loop() {
  int state = digitalRead(PIR);

  if(state == HIGH){
    Serial.println("Motion Dected");
  }
  else{
    Serial.println("Motion not detected");
  }

  delay(200);

}

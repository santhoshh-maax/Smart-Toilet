#define BUZZER 32

void setup(){
  Serial.begin(9600);
  pinMode(BUZZER, OUTPUT);
}

void loop(){
  digitalWrite(BUZZER, HIGH);
  Serial.println("BUZZER ON");
  delay(1000);

  digitalWrite(BUZZER, LOW);
  Serial.println("BUZZER OFF");
  delay(1000);
}
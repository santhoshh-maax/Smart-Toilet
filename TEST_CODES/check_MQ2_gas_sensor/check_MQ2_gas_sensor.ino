#define MQ2_PIN 34

void setup(){
  Serial.begin(9600);
}

void loop(){
  int value = analogRead(MQ2_PIN);

  Serial.println("Gas Value: ");
  Serial.print(value);

  delay(1000);

}
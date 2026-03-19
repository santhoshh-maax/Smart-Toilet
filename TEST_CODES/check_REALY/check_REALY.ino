#define RELAY 26

void setup(){
  Serial.begin(9600);
  pinMode(RELAY, OUTPUT);
}

void loop(){
  digitalWrite(RELAY, LOW); // LOW = ON
  Serial.println("REALY ON");
  delay(2000);

  digitalWrite(RELAY, HIGH); //HIGH = OFF
  Serial.println("REALY OFF");
  delay(2000);

}
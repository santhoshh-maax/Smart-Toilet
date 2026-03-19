#define TRI 12
#define ECHO 13
void setup() {
  Serial.begin(9600);
  pinMode(TRI, OUTPUT);
  pinMode(ECHO, INPUT);

}

void loop() {
  digitalWrite(TRI, LOW);
  delayMicroseconds(2);

  digitalWrite(TRI, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRI, LOW);

  long duration = pulseIn(ECHO, HIGH);
  float distance = duration * 0.034 / 2;

  Serial.println("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  delay(200);
}

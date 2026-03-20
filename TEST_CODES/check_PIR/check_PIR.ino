#define PIR 14

void setup() {
  Serial.begin(115200);
  pinMode(PIR, INPUT);
}

void loop() {
  int state = digitalRead(PIR);

  if (state == HIGH) {
    Serial.println("Motion Detected");
    // delay(1000);  // delay to stabilize
  } else {
    Serial.println("No Motion");
  }

  delay(200);
}
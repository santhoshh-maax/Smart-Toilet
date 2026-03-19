#include <DHT.h>

#define DHT_PIN 27
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

void setup(){
  Serial.begin(9600);
  dht.begin();
}

void loop(){
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  Serial.println("Temp: ");
  Serial.print(temp);
  Serial.print(" °C  Humidity: ");
  Serial.println(hum);

  delay(1000);
}
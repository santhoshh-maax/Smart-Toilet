#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set LCD address (usually 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
  lcd.begin();        // initialize LCD
  lcd.backlight();    // turn on backlight

  lcd.setCursor(0,0);
  lcd.print("Hello World!");

  lcd.setCursor(0,1);
  lcd.print("LCD Working");
}

void loop()
{
  // nothing here
}
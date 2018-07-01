#include <Wiegand.h>
#include <LiquidCrystal.h>

WIEGAND wg;
LiquidCrystal lcd(9, 8, 4, 5, 6, 7);
uint8_t fc;
uint16_t cn;

int ledPin = 11;
int butPin = 10;

boolean curBut = false;
boolean prevBut = false;
boolean ButPressed = false;

boolean validData = false;

void setup() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    pinMode(butPin, INPUT_PULLUP);
    
    Serial.begin(9600);  
    wg.begin();
    lcd.begin(8, 2);
    lcd.clear();
    lcd.print("Scan");
}

void loop() {
  //delay(50);
  if (digitalRead(butPin) == 0) {
    curBut = true;
  } else {
    curBut = false;
  }
  ButPressed = !curBut && prevBut;
  prevBut = curBut;
  
  if(wg.available()) {
    fc = wg.getCode() >> 16;
    cn = wg.getCode() & 0xFFFF;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("W");
    lcd.print(wg.getWiegandType());
    lcd.setCursor(0, 1);
    lcd.print(cn);
    validData = true;
      /*
      Keyboard.begin();
      Keyboard.print(cn);
      Keyboard.end();
      */
      /*
      Serial.print("Facility Code = ");
      Serial.print(fc);
      Serial.print("Card Number = ");
      Serial.print(cn);
      Serial.print(", Type W");
      Serial.println(wg.getWiegandType());  
      */  
    }
    
    if (validData) {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
    
    if (validData && ButPressed) {
      Keyboard.begin();
      Keyboard.print(cn);
      Keyboard.end();
      validData = false;
      lcd.clear();
      lcd.print("Scan");
    }
}

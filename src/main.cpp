#include <Arduino.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
const byte RED = 25;
const byte GREEN = 26;
const byte BLUE = 27;

const byte num_rows = 4;
const byte num_cols = 4;

byte row_pins[num_rows] = {13, 4, 14, 33};
byte col_pins[num_cols] = {32, 19, 23, 5};

char hexaKeys[num_rows][num_cols] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

Keypad k = Keypad(makeKeymap(hexaKeys), row_pins, col_pins, num_rows, num_cols);

const byte BUZZER = 18;

LiquidCrystal_I2C lcd(0x27, 16, 2);
void setup()
{
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  lcd.init();
  lcd.backlight();
}

void loop()
{
  char key = k.getKey();
  if (key != NO_KEY)
  {
    Serial.println(key);
    switch (key)
    {
    case '1':
      digitalWrite(RED, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("RED ON ");
      break;
    case '2':
      digitalWrite(GREEN, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("GREEN ON ");
      break;
    case '3':
      digitalWrite(BLUE, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("BLUE ON ");
      break;
    case 'A':
      digitalWrite(RED, LOW);
      lcd.setCursor(0, 1);
      lcd.print("RED OFF ");
      break;
    case 'B':
      digitalWrite(GREEN, LOW);
      lcd.setCursor(0, 1);
      lcd.print("GREEN OFF ");
      break;
    case 'C':
      digitalWrite(BLUE, LOW);
      lcd.setCursor(0, 1);
      lcd.print("BLUE OFF ");
      break;
    case '4':
      tone(BUZZER, 1000);
      lcd.setCursor(0, 1);
      lcd.print("BUZZER ON ");
      break;
    case '5':
      noTone(BUZZER);
      lcd.setCursor(0, 1);
      lcd.print("BUZZER OFF ");
      break;
    default:
      break;
    }
    lcd.setCursor(0, 0);
    lcd.print("Key Pressed: ");
    lcd.print(key);
  }
}
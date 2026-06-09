#include <Arduino.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <config.h>

#include <input_manager.h>
#include <access_validator.h>
#include <state_manager.h>
#include <display_manager.h>
#include <feedback.h>

// LCD
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

// Keypad Layout
char keys[ROWS][COLS] =
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}};

Keypad keypad(
    makeKeymap(keys),
    (byte *)ROW_PINS,
    (byte *)COL_PINS,
    ROWS,
    COLS);

// Managers
InputManager inputManager;

AccessValidator validator;

StateMachine stateMachine(
    &inputManager,
    &validator);

DisplayManager displayManager(&lcd);

Feedback feedback(
    RED_PIN,
    GREEN_PIN,
    BLUE_PIN,
    BUZZER_PIN);

void setup()
{
  Serial.begin(115200);

  Wire.begin(LCD_SDA, LCD_SCL);

  lcd.init();
  lcd.backlight();

  displayManager.showWelcome();
}

void loop()
{
  stateMachine.update();

  char key = keypad.getKey();

  if (key)
  {
    Serial.print("Key: ");
    Serial.println(key);

    stateMachine.handleKey(key);
  }

  switch (stateMachine.getState())
  {
  case IDLE:

    displayManager.showInput(
        inputManager.length());

    break;

  case GRANTED:

    displayManager.showGranted(
        stateMachine.getCurrentRole());

    feedback.success();

    delay(2000);

    feedback.off();

    stateMachine.reset();

    displayManager.showWelcome();

    break;

  case DENIED:

    displayManager.showDenied();

    feedback.denied();

    delay(2000);

    feedback.off();

    stateMachine.reset();

    displayManager.showWelcome();

    break;

  case LOCKED:

    displayManager.showLocked();

    feedback.locked();

    break;

  case VALIDATING:
    break;
  }
}
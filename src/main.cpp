#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// -----------------------------------------------------------------------------
// System Settings
// -----------------------------------------------------------------------------

const int MAX_ID_LENGTH = 4;
const int FAIL_SAFE_COUNTER = 3;
const unsigned long LOCKOUT_TIME = 15000;

// RGB LED
const byte RED_PIN = 25;
const byte GREEN_PIN = 26;
const byte BLUE_PIN = 27;

// Buzzer
const byte BUZZER_PIN = 18;

// LCD I2C
const byte LCD_SDA = 21;
const byte LCD_SCL = 22;
const byte LCD_ADDR = 0x27;

// Keypad
const byte ROWS = 4;
const byte COLS = 4;

const byte ROW_PINS[ROWS] = {
    13,
    4,
    14,
    33};

const byte COL_PINS[COLS] = {
    32,
    19,
    23,
    5};

struct User
{
  String id;
  String role;
};

const User users[] = {
    {"1234", "admin"},
    {"6789", "security"},
    {"5432", "manager"},
    {"0987", "user-1"},
    {"1122", "user-2"},
};

// -----------------------------------------------------------------------------
// Hardware Objects
// -----------------------------------------------------------------------------

LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

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

// -----------------------------------------------------------------------------
// Global Input Buffer Helpers
// -----------------------------------------------------------------------------

String inputBuffer;

void clearInputBuffer()
{
  inputBuffer = "";
}

void addKeyToInputBuffer(char key)
{
  if (key < '0' || key > '9')
  {
    return;
  }

  if (inputBuffer.length() >= MAX_ID_LENGTH)
  {
    return;
  }

  inputBuffer += key;
}

void backspaceInputBuffer()
{
  if (inputBuffer.length() != 0)
  {
    inputBuffer.remove(inputBuffer.length() - 1);
  }
}

String getInputBuffer()
{
  return inputBuffer;
}

int getInputBufferLength()
{
  return inputBuffer.length();
}

// -----------------------------------------------------------------------------
// Access Validation
// -----------------------------------------------------------------------------

String validateAccess(const String &id)
{
  for (const User &user : users)
  {
    if (user.id == id)
    {
      return user.role;
    }
  }

  return "";
}

// -----------------------------------------------------------------------------
// Display Helpers
// -----------------------------------------------------------------------------

enum DisplayScreen
{
  DS_NONE = 0,
  DS_WELCOME,
  DS_INPUT,
  DS_GRANTED,
  DS_DENIED,
  DS_LOCKED
};

DisplayScreen lastScreen = DS_NONE;
int lastInputLength = -1;
int lastLockedSeconds = -1;

void clearDisplayCache()
{
  lcd.clear();
  lastInputLength = -1;
  lastLockedSeconds = -1;
  lastScreen = DS_NONE;
}

void showWelcome()
{
  if (lastScreen == DS_WELCOME)
  {
    return;
  }

  lcd.clear();

  lastInputLength = -1;
  lastLockedSeconds = -1;
  lastScreen = DS_WELCOME;

  lcd.setCursor(0, 1);
  lcd.print("Enter ID:");
  lcd.setCursor(0, 0);
  lcd.print("#-Enter D-Delete");
}

void showInput(int length)
{
  if (lastScreen == DS_INPUT && length == lastInputLength)
  {
    return;
  }

  if (lastScreen != DS_INPUT)
  {
    lcd.setCursor(0, 0);
    for (int c = 0; c < 20; c++)
    {
      lcd.print(' ');
    }
    lcd.setCursor(0, 0);
    lcd.print("#-Enter D-Delete");

    lcd.setCursor(0, 1);
    lcd.print("Enter ID:");
  }

  lcd.setCursor(9, 1);
  for (int c = 0; c < 20; c++)
  {
    lcd.print(' ');
  }

  lcd.setCursor(9, 1);
  for (int i = 0; i < length; i++)
  {
    lcd.print("*");
  }

  lastInputLength = length;
  lastScreen = DS_INPUT;
}

void showGranted(const String &role)
{
  if (lastScreen == DS_GRANTED)
  {
    return;
  }

  lcd.clear();
  lastInputLength = -1;
  lastLockedSeconds = -1;
  lastScreen = DS_GRANTED;

  lcd.setCursor(0, 0);
  lcd.print("Access Granted");

  lcd.setCursor(0, 1);
  lcd.print("Role: ");
  lcd.print(role);
}

void showDenied()
{
  if (lastScreen == DS_DENIED)
  {
    return;
  }

  lcd.clear();
  lastInputLength = -1;
  lastLockedSeconds = -1;
  lastScreen = DS_DENIED;

  lcd.setCursor(0, 0);
  lcd.print("Access Denied");
}

void showLocked(unsigned long remainingSeconds)
{
  if (lastScreen == DS_LOCKED && remainingSeconds == static_cast<unsigned long>(lastLockedSeconds))
  {
    return;
  }

  lcd.clear();
  lastInputLength = -1;
  lastLockedSeconds = static_cast<int>(remainingSeconds);
  lastScreen = DS_LOCKED;

  lcd.setCursor(0, 0);
  lcd.print("SYSTEM LOCKED");

  lcd.setCursor(0, 1);
  lcd.print("Try in ");
  lcd.print(remainingSeconds);
  lcd.print(" sec");
}

// -----------------------------------------------------------------------------
// Feedback Helpers
// -----------------------------------------------------------------------------

void feedbackOff()
{
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void feedbackSuccess()
{
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
}

void feedbackDenied()
{
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);

  for (int i = 0; i < 2; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

void feedbackLocked()
{
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
}

void initFeedback()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

// -----------------------------------------------------------------------------
// Global System State
// -----------------------------------------------------------------------------

enum SystemState
{
  S_IDLE,
  S_VALIDATING,
  S_GRANTED,
  S_DENIED,
  S_LOCKED
};

SystemState currentState = S_IDLE;
int failCount = 0;
String currentRole = "";
unsigned long lockoutStartTime = 0;

void resetSystem()
{
  currentState = S_IDLE;
  currentRole = "";
  clearInputBuffer();
}

String getCurrentRole()
{
  return currentRole;
}

SystemState getState()
{
  return currentState;
}

unsigned long getLockoutRemainingSeconds()
{
  if (currentState != S_LOCKED)
  {
    return 0;
  }

  unsigned long elapsed = millis() - lockoutStartTime;
  if (elapsed >= LOCKOUT_TIME)
  {
    return 0;
  }

  unsigned long remainingMillis = LOCKOUT_TIME - elapsed;
  return (remainingMillis + 999) / 1000;
}

void grantAccess(const String &role)
{
  currentRole = role;

  failCount = 0;

  currentState = S_GRANTED;

  clearInputBuffer();
}

void denyAccess()
{
  failCount++;

  clearInputBuffer();

  if (failCount >= FAIL_SAFE_COUNTER)
  {
    currentState = S_LOCKED;
    lockoutStartTime = millis();
  }
  else
  {
    currentState = S_DENIED;
  }
}

void validateCurrentInput()
{
  String role = validateAccess(getInputBuffer());

  if (role != "")
  {
    grantAccess(role);
  }
  else
  {
    denyAccess();
  }
}

void handleKey(char key)
{
  if (currentState == S_LOCKED)
  {
    return;
  }

  if (key == 'D')
  {
    backspaceInputBuffer();
    return;
  }

  if (key == '#')
  {
    currentState = S_VALIDATING;
    validateCurrentInput();
    return;
  }

  if (key >= '0' && key <= '9')
  {
    addKeyToInputBuffer(key);
    currentState = S_IDLE;
    return;
  }
}

void updateStateMachine()
{
  if (currentState == S_LOCKED)
  {
    if (millis() - lockoutStartTime >= LOCKOUT_TIME)
    {
      failCount = 0;
      resetSystem();
    }
  }
}

// -----------------------------------------------------------------------------
// Arduino Lifecycle
// -----------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  Wire.begin(LCD_SDA, LCD_SCL);

  lcd.init();
  lcd.backlight();

  initFeedback();

  showWelcome();
}

void loop()
{
  updateStateMachine();

  char key = keypad.getKey();

  if (key)
  {
    Serial.print("Key: ");
    Serial.println(key);

    handleKey(key);
  }

  switch (getState())
  {
  case S_IDLE:
    showInput(getInputBufferLength());
    break;

  case S_GRANTED:
    showGranted(getCurrentRole());
    feedbackSuccess();

    delay(2000);

    feedbackOff();

    resetSystem();

    showWelcome();
    break;

  case S_DENIED:
    showDenied();
    feedbackDenied();

    delay(2000);

    feedbackOff();

    resetSystem();

    showWelcome();
    break;

  case S_LOCKED:
    showLocked(getLockoutRemainingSeconds());
    feedbackLocked();
    break;

  case S_VALIDATING:
    break;
  }
}
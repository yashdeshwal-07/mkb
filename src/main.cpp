#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>

// -----------------------------------------------------------------------------
// System Settings
// -----------------------------------------------------------------------------

const int MAX_ID_LENGTH = 4;
const int FAIL_SAFE_COUNTER = 3;
const unsigned long LOCKOUT_TIME = 15000;
const unsigned long RESULT_SCREEN_TIME = 2000;
const unsigned long SUCCESS_BUZZ_TIME = 200;
const unsigned long DENIED_BEEP_ON_TIME = 100;
const unsigned long DENIED_BEEP_OFF_TIME = 100;
const unsigned long LOCKED_BEEP_PERIOD = 1000;
const unsigned long LOCKED_BEEP_ON_TIME = 100;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000;
const unsigned long TIME_CACHE_REFRESH_INTERVAL = 1000;

// WiFi
#ifndef WIFI_SSID
#define WIFI_SSID "Wokwi-GUEST"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

// Time / logging
const char *NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SECONDS = 0;
const int DAYLIGHT_OFFSET_SECONDS = 0;

#ifndef ACCESS_LOG_URL
#define ACCESS_LOG_URL "http://192.168.1.100:8080/access-log"
#endif

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
// Forward Declarations for Phase 2 Helpers
// -----------------------------------------------------------------------------

bool isWiFiConnected();
void initTime();
void updateTimeCache();
String getCurrentTime();
void logAccess(const String &id, const String &role, const String &status);
void flushOfflineQueue();

// -----------------------------------------------------------------------------
// Global Input Buffer Helpers
// -----------------------------------------------------------------------------

String inputBuffer;
unsigned long lastInputTime = 0;

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
  lastInputTime = millis();
}

void backspaceInputBuffer()
{
  if (inputBuffer.length() != 0)
  {
    inputBuffer.remove(inputBuffer.length() - 1);
    lastInputTime = millis();
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
// WiFi Helpers
// -----------------------------------------------------------------------------

unsigned long lastWiFiReconnectAttempt = 0;
bool wifiWasConnected = false;

void connectWiFi()
{
  if (isWiFiConnected())
  {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWiFiReconnectAttempt = millis();
}

void reconnectWiFi()
{
  if (isWiFiConnected())
  {
    return;
  }

  if (millis() - lastWiFiReconnectAttempt < WIFI_RECONNECT_INTERVAL)
  {
    return;
  }

  Serial.println("[WiFi] Reconnecting...");
  connectWiFi();
}

bool isWiFiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void updateWiFiStatus()
{
  bool connected = isWiFiConnected();

  if (connected == wifiWasConnected)
  {
    return;
  }

  wifiWasConnected = connected;

  if (connected)
  {
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("[WiFi] Disconnected");
  }
}

// -----------------------------------------------------------------------------
// NTP Time Helpers
// -----------------------------------------------------------------------------

bool timeConfigured = false;
bool timeCacheReady = false;
unsigned long lastTimeCacheRefresh = 0;
time_t currentEpoch = 0;
String currentDateTime = "TIME_NOT_SYNCED";

void initTime()
{
  if (timeConfigured)
  {
    return;
  }

  configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, NTP_SERVER);
  timeConfigured = true;

  Serial.println("[NTP] Time synchronization requested");
}

void updateTimeCache()
{
  if (!timeConfigured)
  {
    return;
  }

  if (millis() - lastTimeCacheRefresh < TIME_CACHE_REFRESH_INTERVAL && timeCacheReady)
  {
    return;
  }

  time_t now = time(nullptr);
  if (now < 100000)
  {
    return;
  }

  struct tm timeInfo;
  localtime_r(&now, &timeInfo);

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

  currentEpoch = now;
  currentDateTime = buffer;
  lastTimeCacheRefresh = millis();
  timeCacheReady = true;
}

String getCurrentTime()
{
  updateTimeCache();
  return currentDateTime;
}

// -----------------------------------------------------------------------------
// HTTP Access Log Helpers
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Offline Queue & Sending Helpers
// -----------------------------------------------------------------------------

struct LogEntry
{
  String id;
  String role;
  String status;
  String timestamp;
};

const int MAX_QUEUE_SIZE = 10;
LogEntry offlineQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

unsigned long lastFlushAttempt = 0;
const unsigned long FLUSH_INTERVAL = 10000; // 10 seconds

void enqueueLog(const String &id, const String &role, const String &status, const String &timestamp)
{
  if (queueCount >= MAX_QUEUE_SIZE)
  {
    Serial.println("[Queue] Queue full, overwriting oldest entry");
    queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
    queueCount--;
  }

  offlineQueue[queueTail] = {id, role, status, timestamp};
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueCount++;
  Serial.print("[Queue] Log enqueued. Count: ");
  Serial.println(queueCount);
}

bool peekLog(LogEntry &entry)
{
  if (queueCount == 0)
  {
    return false;
  }
  entry = offlineQueue[queueHead];
  return true;
}

void popLog()
{
  if (queueCount == 0)
  {
    return;
  }
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  queueCount--;
  Serial.print("[Queue] Log popped. Count: ");
  Serial.println(queueCount);
}

bool sendPayload(const String &id, const String &role, const String &status, const String &timestamp)
{
  if (!isWiFiConnected())
  {
    return false;
  }

  JsonDocument doc;
  doc["id"] = id;
  doc["role"] = role;
  doc["status"] = status;
  doc["time"] = timestamp;

  String payload;
  serializeJson(doc, payload);

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(1500);

  if (!http.begin(client, ACCESS_LOG_URL))
  {
    Serial.println("[HTTP] Failed to initialize request");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);
  bool success = false;
  if (httpCode >= 200 && httpCode < 300)
  {
    Serial.print("[HTTP] Log sent successfully, code: ");
    Serial.println(httpCode);
    success = true;
  }
  else
  {
    Serial.print("[HTTP] Log send failed, code/error: ");
    if (httpCode > 0)
    {
      Serial.println(httpCode);
    }
    else
    {
      Serial.println(http.errorToString(httpCode));
    }
  }

  http.end();
  return success;
}

void logAccess(const String &id, const String &role, const String &status)
{
  String timestamp = getCurrentTime();

  if (queueCount == 0 && isWiFiConnected())
  {
    Serial.println("[Logging] Queue empty, attempting immediate send");
    if (sendPayload(id, role, status, timestamp))
    {
      return;
    }
    Serial.println("[Logging] Immediate send failed, queueing log");
  }
  else
  {
    Serial.println("[Logging] WiFi offline or queue not empty, queueing log");
  }

  enqueueLog(id, role, status, timestamp);
}

void flushOfflineQueue()
{
  if (queueCount == 0)
  {
    return;
  }

  if (!isWiFiConnected())
  {
    return;
  }

  if (millis() - lastFlushAttempt < FLUSH_INTERVAL)
  {
    return;
  }

  lastFlushAttempt = millis();

  Serial.print("[Queue] Flushing ");
  Serial.print(queueCount);
  Serial.println(" logs...");

  while (queueCount > 0 && isWiFiConnected())
  {
    LogEntry entry;
    if (peekLog(entry))
    {
      if (sendPayload(entry.id, entry.role, entry.status, entry.timestamp))
      {
        popLog();
      }
      else
      {
        Serial.println("[Queue] Send failed during flush, aborting flush");
        break;
      }
    }
  }
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
// Global State Timers
// -----------------------------------------------------------------------------

unsigned long currentStateEntryTime = 0;
unsigned long lockoutStartTime = 0;

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
  unsigned long elapsed = millis() - currentStateEntryTime;

  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);

  digitalWrite(BUZZER_PIN, elapsed < SUCCESS_BUZZ_TIME ? HIGH : LOW);
}

void feedbackDenied()
{
  unsigned long elapsed = millis() - currentStateEntryTime;

  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);

  if (elapsed < DENIED_BEEP_ON_TIME)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (elapsed < DENIED_BEEP_ON_TIME + DENIED_BEEP_OFF_TIME)
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
  else if (elapsed < (DENIED_BEEP_ON_TIME * 2) + DENIED_BEEP_OFF_TIME)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void feedbackLocked()
{
  unsigned long elapsed = millis() - lockoutStartTime;
  unsigned long beepTime = elapsed % LOCKED_BEEP_PERIOD;

  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);

  digitalWrite(BUZZER_PIN, beepTime < LOCKED_BEEP_ON_TIME ? HIGH : LOW);
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

void resetSystem()
{
  currentState = S_IDLE;
  currentRole = "";
  currentStateEntryTime = millis();
  feedbackOff();
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
  currentStateEntryTime = millis();

  logAccess(getInputBuffer(), role, "GRANTED");

  clearInputBuffer();
}

void denyAccess()
{
  failCount++;

  if (failCount >= FAIL_SAFE_COUNTER)
  {
    currentState = S_LOCKED;
    lockoutStartTime = millis();
    currentStateEntryTime = lockoutStartTime;
  }
  else
  {
    currentState = S_DENIED;
    currentStateEntryTime = millis();
  }

  logAccess(getInputBuffer(), "", "DENIED");

  clearInputBuffer();
}

void validateCurrentInput()
{
  String id = getInputBuffer();

  String role = validateAccess(id);

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
    if (getInputBufferLength() == 0)
    {
      return;
    }
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

void updateInputTimeout()
{
  if (!inputBuffer.isEmpty())
  {
    if (millis() - lastInputTime >= 10000)
    {
      clearInputBuffer();
    }
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

  connectWiFi();
  initTime();

  showWelcome();
}

void loop()
{
  reconnectWiFi();
  updateWiFiStatus();
  updateTimeCache();
  flushOfflineQueue();

  updateStateMachine();
  updateInputTimeout();

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

    if (millis() - currentStateEntryTime >= RESULT_SCREEN_TIME)
    {
      resetSystem();
      showWelcome();
    }
    break;

  case S_DENIED:
    showDenied();
    feedbackDenied();

    if (millis() - currentStateEntryTime >= RESULT_SCREEN_TIME)
    {
      resetSystem();
      showWelcome();
    }
    break;

  case S_LOCKED:
    showLocked(getLockoutRemainingSeconds());
    feedbackLocked();
    break;

  case S_VALIDATING:
    break;
  }
}
#pragma once

#include <Arduino.h>

// System Settings
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
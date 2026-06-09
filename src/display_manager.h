#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class DisplayManager
{
private:
    LiquidCrystal_I2C *lcd;
    int lastInputLength;
    int lastLockedSeconds;
    enum DisplayScreen
    {
        DS_NONE = 0,
        DS_WELCOME,
        DS_INPUT,
        DS_GRANTED,
        DS_DENIED,
        DS_LOCKED
    };
    DisplayScreen lastScreen;

public:
    DisplayManager(LiquidCrystal_I2C *lcd);

    void showWelcome();

    void showInput(int length);

    void showGranted(const String &role);

    void showDenied();

    void showLocked(unsigned long remainingSeconds);

    void clear();
};
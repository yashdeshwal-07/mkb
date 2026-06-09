#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class DisplayManager
{
private:
    LiquidCrystal_I2C *lcd;

public:
    DisplayManager(LiquidCrystal_I2C *lcd);

    void showWelcome();

    void showInput(int length);

    void showGranted(const String &role);

    void showDenied();

    void showLocked();

    void clear();
};
#include "display_manager.h"

DisplayManager::DisplayManager(
    LiquidCrystal_I2C *lcd)
{
    this->lcd = lcd;
}

void DisplayManager::clear()
{
    lcd->clear();
}

void DisplayManager::showWelcome()
{
    lcd->clear();

    lcd->setCursor(0, 0);
    lcd->print("Enter ID:");
}

void DisplayManager::showInput(int length)
{
    lcd->clear();

    lcd->setCursor(0, 0);
    lcd->print("Enter ID:");

    lcd->setCursor(0, 1);

    for (int i = 0; i < length; i++)
    {
        lcd->print("*");
    }
}

void DisplayManager::showGranted(
    const String &role)
{
    lcd->clear();

    lcd->setCursor(0, 0);
    lcd->print("Access Granted");

    lcd->setCursor(0, 1);
    lcd->print("Role: ");
    lcd->print(role);
}

void DisplayManager::showDenied()
{
    lcd->clear();

    lcd->setCursor(0, 0);
    lcd->print("Access Denied");
}

void DisplayManager::showLocked()
{
    lcd->clear();

    lcd->setCursor(0, 0);
    lcd->print("SYSTEM LOCKED");

    lcd->setCursor(0, 1);
    lcd->print("Try Again Later");
}
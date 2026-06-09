#include <display_manager.h>

DisplayManager::DisplayManager(
    LiquidCrystal_I2C *lcd)
{
    this->lcd = lcd;
    this->lastInputLength = -1;
    this->lastLockedSeconds = -1;
    this->lastScreen = DS_NONE;
}

void DisplayManager::clear()
{
    lcd->clear();
    lastInputLength = -1;
    lastLockedSeconds = -1;
    lastScreen = DS_NONE;
}

void DisplayManager::showWelcome()
{
    if (lastScreen == DS_WELCOME)
    {
        return;
    }

    lcd->clear();

    lastInputLength = -1;
    lastLockedSeconds = -1;
    lastScreen = DS_WELCOME;

    lcd->setCursor(0, 1);
    lcd->print("Enter ID:");
    lcd->setCursor(0, 0);
    lcd->print("#-Enter D-Delete");
}

void DisplayManager::showInput(int length)
{
    if (lastScreen == DS_INPUT && length == lastInputLength)
    {
        return;
    }

    if (lastScreen != DS_INPUT)
    {
        lcd->setCursor(0, 0);
        for (int c = 0; c < 20; c++)
        {
            lcd->print(' ');
        }
        lcd->setCursor(0, 0);
        lcd->print("#-Enter D-Delete");

        lcd->setCursor(0, 1);
        lcd->print("Enter ID:");
    }

    lcd->setCursor(9, 1);
    for (int c = 0; c < 20; c++)
    {
        lcd->print(' ');
    }

    lcd->setCursor(9, 1);
    for (int i = 0; i < length; i++)
    {
        lcd->print("*");
    }

    lastInputLength = length;
    lastScreen = DS_INPUT;
}

void DisplayManager::showGranted(
    const String &role)
{
    if (lastScreen == DS_GRANTED)
    {
        return;
    }

    lcd->clear();
    lastInputLength = -1;
    lastLockedSeconds = -1;
    lastScreen = DS_GRANTED;

    lcd->setCursor(0, 0);
    lcd->print("Access Granted");

    lcd->setCursor(0, 1);
    lcd->print("Role: ");
    lcd->print(role);
}

void DisplayManager::showDenied()
{
    if (lastScreen == DS_DENIED)
    {
        return;
    }

    lcd->clear();
    lastInputLength = -1;
    lastLockedSeconds = -1;
    lastScreen = DS_DENIED;

    lcd->setCursor(0, 0);
    lcd->print("Access Denied");
}

void DisplayManager::showLocked(unsigned long remainingSeconds)
{
    if (lastScreen == DS_LOCKED && remainingSeconds == lastLockedSeconds)
    {
        return;
    }

    lcd->clear();
    lastInputLength = -1;
    lastLockedSeconds = static_cast<int>(remainingSeconds);
    lastScreen = DS_LOCKED;

    lcd->setCursor(0, 0);
    lcd->print("SYSTEM LOCKED");

    lcd->setCursor(0, 1);
    lcd->print("Try in ");
    lcd->print(remainingSeconds);
    lcd->print(" sec");
}
#pragma once

#include <Arduino.h>

class Feedback
{
private:
    int redPin;
    int greenPin;
    int bluePin;

    int buzzerPin;

public:
    Feedback(
        int redPin,
        int greenPin,
        int bluePin,
        int buzzerPin);

    void success();

    void denied();

    void locked();

    void off();
};
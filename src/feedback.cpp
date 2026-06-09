#include "feedback.h"

Feedback::Feedback(
    int redPin,
    int greenPin,
    int bluePin,
    int buzzerPin)
{
    this->redPin = redPin;
    this->greenPin = greenPin;
    this->bluePin = bluePin;
    this->buzzerPin = buzzerPin;

    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);

    pinMode(buzzerPin, OUTPUT);
}
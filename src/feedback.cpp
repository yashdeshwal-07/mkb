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

void Feedback::success()
{
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);
    digitalWrite(bluePin, LOW);

    digitalWrite(buzzerPin, HIGH);
    delay(150);
    digitalWrite(buzzerPin, LOW);
}

void Feedback::denied()
{
    // brief red flash + short buzz
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
    digitalWrite(redPin, HIGH);

    for (int i = 0; i < 2; i++)
    {
        digitalWrite(buzzerPin, HIGH);
        delay(100);
        digitalWrite(buzzerPin, LOW);
        delay(100);
    }
}

void Feedback::locked()
{
    // steady red, prolonged buzz
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
    digitalWrite(redPin, HIGH);

    digitalWrite(buzzerPin, HIGH);
    delay(300);
    digitalWrite(buzzerPin, LOW);
}

void Feedback::off()
{
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
    digitalWrite(buzzerPin, LOW);
}
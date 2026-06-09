#pragma once

#include <Arduino.h>

class InputManager
{
private:
    String buffer;

public:
    InputManager();

    void addKey(char key);
    void backspace();
    void clear();

    String getBuffer() const;

    int length() const;
    bool isFull() const;
    bool isEmpty() const;
};
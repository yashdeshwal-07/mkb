#pragma once

#include <Arduino.h>

class AccessValidator
{
public:
    String validate(const String &id);
};
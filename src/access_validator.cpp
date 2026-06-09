#include <access_validator.h>
#include <users.h>
#include <Arduino.h>

String AccessValidator::validate(const String &id)
{
    for (const User &user : users)
    {
        if (user.id == id)
        {
            return user.role;
        }
    }
    return "";
}
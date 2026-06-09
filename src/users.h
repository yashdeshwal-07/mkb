#pragma once

#include <Arduino.h>

struct User
{
    String id;
    String role;
};

const User users[] = {
    {"1234", "admin"},
    {"6789", "security"},
    {"5432", "manager"},
    {"0987", "user-1"},
    {"1122", "user-2"},
};
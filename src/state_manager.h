#pragma once

#include <Arduino.h>
#include "input_manager.h"
#include "access_validator.h"

enum SystemState
{
    S_IDLE,
    S_VALIDATING,
    S_GRANTED,
    S_DENIED,
    S_LOCKED
};

class StateMachine
{
private:
    SystemState currentState;

    int failCount;
    String currentRole;

    unsigned long lockoutStartTime;

    InputManager *inputManager;
    AccessValidator *validator;

    void validateCurrentInput();
    void grantAccess(const String &role);
    void denyAccess();

public:
    StateMachine(
        InputManager *inputManager,
        AccessValidator *validator);

    void handleKey(char key);
    void update();

    SystemState getState() const;
    String getCurrentRole() const;

    void reset();
};
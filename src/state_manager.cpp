#include <state_manager.h>
#include <config.h>
#include <Arduino.h>
StateMachine::StateMachine(
    InputManager *inputManager,
    AccessValidator *validator)
{
    this->inputManager = inputManager;
    this->validator = validator;

    currentState = S_IDLE;
    failCount = 0;
    currentRole = "";
    lockoutStartTime = 0;
}

SystemState StateMachine::getState() const
{
    return currentState;
}

String StateMachine::getCurrentRole() const
{
    return currentRole;
}

void StateMachine::reset()
{
    currentState = S_IDLE;
    currentRole = "";
    inputManager->clear();
}

void StateMachine::grantAccess(const String &role)
{
    currentRole = role;

    failCount = 0;

    currentState = S_GRANTED;

    inputManager->clear();
}

void StateMachine::denyAccess()
{
    failCount++;

    inputManager->clear();

    if (failCount >= FAIL_SAFE_COUNTER)
    {
        currentState = S_LOCKED;
        lockoutStartTime = millis();
    }
    else
    {
        currentState = S_DENIED;
    }
}

void StateMachine::validateCurrentInput()
{
    String role =
        validator->validate(
            inputManager->getBuffer());

    if (role != "")
    {
        grantAccess(role);
    }
    else
    {
        denyAccess();
    }
}

void StateMachine::handleKey(char key)
{
    if (currentState == S_LOCKED)
    {
        return;
    }

    if (key == 'D')
    {
        inputManager->backspace();
        return;
    }

    if (key == '#')
    {
        currentState = S_VALIDATING;
        validateCurrentInput();
        return;
    }

    if (key >= '0' && key <= '9')
    {
        inputManager->addKey(key);
        currentState = S_IDLE;
        return;
    }
}
void StateMachine::update()
{
    if (currentState == S_LOCKED)
    {
        if (millis() - lockoutStartTime >= LOCKOUT_TIME)
        {
            failCount = 0;
            reset();
        }
    }
}
#include <state_machine.h>
#include <config.h>
#include <Arduino.h>
StateMachine::StateMachine(
    InputManager *inputManager,
    AccessValidator *validator)
{
    this->inputManager = inputManager;
    this->validator = validator;

    currentState = IDLE;
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
    currentState = IDLE;
    currentRole = "";
    inputManager->clear();
}

void StateMachine::grantAccess(const String &role)
{
    currentRole = role;

    failCount = 0;

    currentState = GRANTED;

    inputManager->clear();
}

void StateMachine::denyAccess()
{
    failCount++;

    inputManager->clear();

    if (failCount >= FAIL_SAFE_COUNTER)
    {
        currentState = LOCKED;
        lockoutStartTime = millis();
    }
    else
    {
        currentState = DENIED;
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
    if (currentState == LOCKED)
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
        currentState = VALIDATING;
        validateCurrentInput();
        return;
    }

    if (key >= '0' && key <= '9')
    {
        inputManager->addKey(key);
        currentState = IDLE;
        return;
    }
}
void StateMachine::update()
{
    if (currentState == LOCKED)
    {
        if (millis() - lockoutStartTime >= LOCKOUT_TIME)
        {
            failCount = 0;
            reset();
        }
    }
}
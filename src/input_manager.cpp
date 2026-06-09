#include <input_manager.h>
#include <config.h>

InputManager::InputManager()
{
    buffer = "";
}

void InputManager::addKey(char key)
{
    // Only digits allowed
    if (key < '0' || key > '9')
        return;

    // Prevent overflow
    if (isFull())
        return;

    buffer += key;
}

void InputManager::backspace()
{
    if (!isEmpty())
    {
        buffer.remove(buffer.length() - 1);
    }
}

void InputManager::clear()
{
    buffer = "";
}

String InputManager::getBuffer() const
{
    return buffer;
}

int InputManager::length() const
{
    return buffer.length();
}

bool InputManager::isFull() const
{
    return buffer.length() >= MAX_ID_LENGTH;
}

bool InputManager::isEmpty() const
{
    return buffer.length() == 0;
}
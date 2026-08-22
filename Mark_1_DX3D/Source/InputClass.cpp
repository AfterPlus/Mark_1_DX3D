#include "InputClass.h"

InputClass::InputClass()
{
}


InputClass::InputClass(const InputClass& other)
{
}


InputClass::~InputClass()
{
}


void InputClass::Initialize()
{
    int i ;
    
    // Initialize all the keys not pressed
    for ( i = 0 ; i < 256 ; i++ )
        m_keys[i] = false;
    
    return ;
}

void InputClass::key_down(unsigned int input)
{
    // if a key is pressed set the key to true
    m_keys[input] = true;
}

void InputClass::key_up(unsigned int input)
{
    // if a key is released set it to false
    m_keys[input] = false;
    return;
}

bool InputClass::is_key_down(unsigned int key)
{
    // Return what state the key is in (pressed/not pressed).
    return m_keys[key];
}

#include "PositionClass.h"

PositionClass::PositionClass()
{
    m_frameTime = 0.0f;
    m_rotationY = 0.0f;
    m_leftTurnSpeed  = 0.0f;
    m_rightTurnSpeed = 0.0f;
}


PositionClass::PositionClass(const PositionClass& other)
{
    
}

PositionClass::~PositionClass()
{
    
}

void PositionClass::SetFrameTime(float time)
{
    m_frameTime = time;
}

void PositionClass::GetRotation(float& y)
{
    y = m_rotationY;
}
 
// TO DO 

void PositionClass::TurnLeft(bool keydown)
{
    // If the key is pressed increase the speed at which the camera turns left.  If not slow down the turn speed.
    if(keydown)
    {
        m_leftTurnSpeed += m_frameTime * 0.01f;

        if(m_leftTurnSpeed > (m_frameTime * 0.1f))
        {
            m_leftTurnSpeed = m_frameTime * 0.1f;
        }
    }
    else
    {
        m_leftTurnSpeed -= m_frameTime * 0.005f;

        if(m_leftTurnSpeed < 0.0f)
        {
            m_leftTurnSpeed = 0.0f;
        }
    }

    // Update the rotation using the turning speed.
    m_rotationY -= m_leftTurnSpeed;
    if(m_rotationY < 0.0f)
        m_rotationY += 360.0f;
}


void PositionClass::TurnRight(bool keydown)
{
    // If the key is pressed increase the speed at which the camera turns right.  If not slow down the turn speed.
    if(keydown)
    {
        m_rightTurnSpeed += m_frameTime * 0.01f;

        if(m_rightTurnSpeed > (m_frameTime * 0.1f))
            m_rightTurnSpeed = m_frameTime * 0.1f;
    }
    else
    {
        m_rightTurnSpeed -= m_frameTime * 0.005f;

        if(m_rightTurnSpeed < 0.0f)
        {
            m_rightTurnSpeed = 0.0f;
        }
    }

    // Update the rotation using the turning speed.
    m_rotationY += m_rightTurnSpeed;
    if(m_rotationY > 360.0f)
    {
        m_rotationY -= 360.0f;
    }
}

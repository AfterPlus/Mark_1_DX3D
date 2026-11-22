#include "TimerClass.h"

TimerClass::TimerClass()
{
    m_frequency = 0.0f;
    m_startTime = 0;
    m_frameTime = 0.0f;
}

TimerClass::TimerClass(const TimerClass& other)
{
}

TimerClass::~TimerClass()
{
}

bool TimerClass::Initialize()
{
    INT64 frequency;

    // Check to see if this system supports high performance timers.
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    if(frequency == 0)
    {
        return false;
    }

    // Find out how many times the frequency counter ticks every second.
    m_frequency = (float)frequency;

    // Get the initial start time.
    QueryPerformanceCounter((LARGE_INTEGER*)&m_startTime);

    return true;
}

void TimerClass::Frame()
{
    INT64 currentTime;
    INT64 elapsedTicks;

    // Query the current time.
    QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

    // Calculate the difference in time since the last time we queried for the current time.
    elapsedTicks = currentTime - m_startTime;

    // Calculate the frame time in seconds.
    m_frameTime = (float)elapsedTicks / m_frequency;

    // Restart the timer.
    m_startTime = currentTime;
}

float TimerClass::GetTime()
{
    return m_frameTime;
}

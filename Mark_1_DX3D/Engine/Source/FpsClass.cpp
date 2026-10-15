#include "FpsClass.h"

FpsClass::FpsClass()
{
    m_fps = 0;
    m_count = 0;
    m_startTime = 0;
}

FpsClass::FpsClass(const FpsClass& other)
{
}

FpsClass::~FpsClass()
{
}

void FpsClass::Initialize()
{
    m_fps = 0;
    m_count = 0;
    m_startTime = timeGetTime();
}

void FpsClass::Frame()
{
    m_count++;

    // Once a second has passed record the number of frames rendered as the fps and start counting again.
    if(timeGetTime() >= (m_startTime + 1000))
    {
        m_fps = m_count;
        m_count = 0;

        m_startTime = timeGetTime();
    }
}

int FpsClass::GetFps()
{
    return m_fps;
}

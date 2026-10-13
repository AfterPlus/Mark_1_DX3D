#pragma once

#include <math.h>
#include <directxmath.h>
using namespace DirectX;

class Car
{
public:
    Car();

    void Reset(float x, float z, float heading);

    // throttle / brake: 0 or 1 (analog later). steerInput: -1 (left) .. +1 (right).
    void Update(float dt, float throttle, float brake, float steerInput);

    XMFLOAT3 GetPosition() const { return XMFLOAT3(m_x, 0.0f, m_z); }
    XMFLOAT3 GetForward() const { return XMFLOAT3(sinf(m_heading), 0.0f, cosf(m_heading)); }
    float GetHeading() const { return m_heading; }   // radians
    float GetSpeed() const { return m_speed; }       // m/s, negative = reversing

private:
    float m_x, m_z;
    float m_heading;
    float m_speed;
    float m_steer;      // smoothed steering, -1..1
};

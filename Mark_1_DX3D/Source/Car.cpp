#include "Car.h"

// Tuning (metres, seconds). Change these while playing to find the feel you want.
static const float MAX_SPEED = 40.0f;           // ~144 km/h
static const float MAX_REVERSE = 10.0f;
static const float ENGINE_ACCEL = 16.0f;        // at standstill; tapers off towards MAX_SPEED
static const float BRAKE_DECEL = 30.0f;
static const float REVERSE_ACCEL = 8.0f;
static const float COAST_DECEL = 5.0f;          // slowdown with no pedal pressed
static const float WHEEL_BASE = 2.6f;
static const float MAX_STEER_ANGLE = 0.5f;      // radians of front wheel angle at standstill
static const float STEER_SPEED_FALLOFF = 0.08f; // the faster, the less the wheels can turn (stability)
static const float STEER_RESPONSE = 8.0f;       // how quickly the wheel follows the key (1/s)

Car::Car()
{
    Reset(0.0f, 0.0f, 0.0f);
}

void Car::Reset(float x, float z, float heading)
{
    m_x = x;
    m_z = z;
    m_heading = heading;
    m_speed = 0.0f;
    m_steer = 0.0f;
}

void Car::Update(float dt, float throttle, float brake, float steerInput)
{
    float blend, maxAngle, steerAngle;

    // 1. Steering wheel eases towards the key instead of snapping.
    blend = STEER_RESPONSE * dt;
    if(blend > 1.0f)
    {
        blend = 1.0f;
    }

    m_steer += (steerInput - m_steer) * blend;

    // 2. Speed along the heading.
    if(throttle > 0.0f)
    {
        if(m_speed < 0.0f)
        {
            m_speed += BRAKE_DECEL * dt;                                    // brake out of reverse first
        }
        else
        {
            m_speed += ENGINE_ACCEL * (1.0f - m_speed / MAX_SPEED) * dt;    // weaker the closer to top speed
        }
    }
    else if(brake > 0.0f)
    {
        if(m_speed > 0.0f)
        {
            m_speed -= BRAKE_DECEL * dt;
            if(m_speed < 0.0f)
            {
                m_speed = 0.0f;                                             // stop first, reverse on the next press
            }
        }
        else
        {
            m_speed -= REVERSE_ACCEL * dt;
        }
    }
    else
    {
        // Coast towards zero from either side.
        if(m_speed > 0.0f)
        {
            m_speed -= COAST_DECEL * dt;
            if(m_speed < 0.0f) { m_speed = 0.0f; }
        }
        else if(m_speed < 0.0f)
        {
            m_speed += COAST_DECEL * dt;
            if(m_speed > 0.0f) { m_speed = 0.0f; }
        }
    }

    if(m_speed > MAX_SPEED)    { m_speed = MAX_SPEED; }
    if(m_speed < -MAX_REVERSE) { m_speed = -MAX_REVERSE; }

    // 3. Bicycle model: the heading turns faster with speed and with front wheel angle.
    maxAngle = MAX_STEER_ANGLE / (1.0f + fabsf(m_speed) * STEER_SPEED_FALLOFF);
    steerAngle = m_steer * maxAngle;

    m_heading += (m_speed / WHEEL_BASE) * tanf(steerAngle) * dt;

    if(m_heading > XM_PI)  { m_heading -= XM_2PI; }
    if(m_heading < -XM_PI) { m_heading += XM_2PI; }

    // 4. Move.
    m_x += sinf(m_heading) * m_speed * dt;
    m_z += cosf(m_heading) * m_speed * dt;
}

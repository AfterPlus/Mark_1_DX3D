#include "DriftScore.h"
#include <math.h>

static const float MIN_ANGLE = 0.14f;           // rad (~8 deg): smaller slides are just cornering
static const float MAX_ANGLE = 1.0f;            // rad (~57 deg): bigger than this is a spin
static const float MIN_SPEED = 12.0f;           // m/s
static const float POINTS_RATE = 0.12f;         // points per second per (degree * m/s): 20 deg at 30 m/s = 72 points/s at x1
static const float GRACE_TIME = 0.8f;           // s between two drifts that still counts as one chain
static const float MULTIPLIER_GROWTH = 0.4f;    // multiplier gained per second of unbroken chain
static const float MULTIPLIER_MAX = 5.0f;

DriftScore::DriftScore()
{
    Reset();
}

void DriftScore::Reset()
{
    m_chain = 0.0f;
    m_chainTime = 0.0f;
    m_multiplier = 1.0f;
    m_graceTimer = 0.0f;
    m_angleDegrees = 0.0f;
    m_drifting = false;
    m_active = false;
    m_total = 0;
    m_bestChain = 0;
}

void DriftScore::Update(float dt, float slipAngle, float speed, bool onRoad)
{
    float angle = fabsf(slipAngle);

    m_angleDegrees = angle * (180.0f / 3.14159265f);

    // Leaving the road in the middle of a chain loses it.
    if(!onRoad && m_active)
    {
        Crash();
    }

    m_drifting = onRoad && speed >= MIN_SPEED && angle >= MIN_ANGLE && angle <= MAX_ANGLE;

    if(m_drifting)
    {
        m_active = true;
        m_graceTimer = GRACE_TIME;
        m_chainTime += dt;

        m_multiplier = 1.0f + MULTIPLIER_GROWTH * m_chainTime;
        if(m_multiplier > MULTIPLIER_MAX)
        {
            m_multiplier = MULTIPLIER_MAX;
        }

        m_chain += m_angleDegrees * speed * POINTS_RATE * m_multiplier * dt;
    }
    else if(m_active)
    {
        m_graceTimer -= dt;

        if(m_graceTimer <= 0.0f)
        {
            Bank();
        }
    }
}

void DriftScore::Crash()
{
    m_chain = 0.0f;
    m_chainTime = 0.0f;
    m_multiplier = 1.0f;
    m_graceTimer = 0.0f;
    m_active = false;
    m_drifting = false;
}

void DriftScore::Bank()
{
    m_total += (int)m_chain;

    if((int)m_chain > m_bestChain)
    {
        m_bestChain = (int)m_chain;
    }

    m_chain = 0.0f;
    m_chainTime = 0.0f;
    m_multiplier = 1.0f;
    m_graceTimer = 0.0f;
    m_active = false;
}

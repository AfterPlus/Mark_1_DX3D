#pragma once

#include <memory>
#include <math.h>
#include <directxmath.h>
#include "../Physics/PhysicsFramework.h"
#include "Road.h"

using namespace DirectX;

struct CarControls
{
    float throttle;     // 0..1
    float brake;        // 0..1
    float steer;        // -1 (left) .. +1 (right)
    float handbrake;    // 0..1
};

// The car is a Physics::RigidBody moving freely on the ground plane (world coordinates).
// Nothing "follows" the road: the engine pushes, the brakes pull, and two tyres (front, rear) turn the car's
// motion into forces using a slip-angle tyre model. Steering turns the FRONT WHEELS; the car then yaws because
// of the forces on the tyres - and when a tyre runs out of grip, the car drifts.
//
// The road is only used to know where the car is on it (s, d) and for the walls beside it.
class Car
{
public:
    Car();

    // s = distance along the road the car is placed at (the road search starts there).
    void Reset(float x, float z, float heading, float speed = 0.0f, float s = 0.0f);

    void Update(float dt, const CarControls& controls, const Road& road);

    XMFLOAT3 GetPosition() const;
    float GetHeading() const { return m_heading; }              // radians, 0 = +Z, + = toward +X
    float GetSpeed() const { return m_forwardSpeed; }           // m/s along the nose
    float GetSlipAngle() const { return m_slipAngle; }          // angle between the nose and the direction of travel, radians (+ = moving right of the nose)
    float GetSteerAngle() const { return m_steerAngle; }        // front wheel angle, radians
    float GetS() const { return m_s; }                          // distance along the road
    float GetD() const { return m_d; }                          // sideways offset from the road centre (+ = right)
    bool IsOffRoad() const { return m_offRoad; }

private:
    void Step(const CarControls& controls);
    void ApplyTyre(float offsetZ, float wheelAngle, float requestedForce, float friction, float load);
    void ApplyWalls(const Road& road);
    void ReadState();

private:
    std::shared_ptr<Physics::Transform> m_transform;
    std::shared_ptr<Physics::RigidBody> m_body;

    // State read from the body at the start of every physics step.
    Physics::v3df m_velocity;
    Physics::v3df m_forward;
    float m_yawRate;

    float m_heading;
    float m_forwardSpeed;
    float m_slipAngle;
    float m_steer;          // smoothed steering input, -1..1
    float m_steerAngle;
    float m_s, m_d;
    bool m_offRoad;
    float m_accumulator;    // leftover time for the fixed-step physics
};

#pragma once

#include <memory>
#include <math.h>
#include <directxmath.h>
#include "Physics/PhysicsFramework.h"
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

    // Powertrain read-outs (HUD / engine sound later).
    float GetRpm() const { return m_rpm; }
    int GetGear() const { return m_gear; }                      // 1..6
    float GetLongitudinalAccel() const { return m_longAccel; }  // m/s^2 along the nose, smoothed (+ = accelerating)

    // True once after the car bounced off a wall (the game uses it to end a drift chain).
    bool ConsumeWallHit() { bool hit = m_wallHit; m_wallHit = false; return hit; }

    // Bicycle-model handling balance of the tuning in Car.cpp (static, no car needed): the understeer gradient K in
    // rad per (m/s^2) of lateral acceleration (> 0 understeer, < 0 oversteer) and the speed at which steering response
    // doubles ('characteristic speed', infinite for a neutral car).
    static void GetHandlingBalance(float& understeerGradient, float& characteristicSpeed);

private:
    void Step(const CarControls& controls);
    void ApplyTyre(float offsetZ, float wheelAngle, float requestedForce, float friction, float load);
    void ApplyWalls(const Road& road);
    void ReadState();
    float UpdateDrivetrain(float throttle, float forwardSpeed);

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

    // Powertrain.
    float m_rpm;
    int m_gear;
    float m_shiftTimer;     // > 0 while changing gear (no torque reaches the wheels)
    float m_gearTimer;      // time since the last gear change

    // Weight transfer.
    float m_longAccel;      // smoothed acceleration along the nose
    bool m_wallHit;

    // Hills: the part of gravity that pulls along the road (uphill slows the car, downhill speeds it up).
    Physics::v3df m_slopeForce;
};

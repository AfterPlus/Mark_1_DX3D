#include "Car.h"

using Physics::v3df;

// ---- Tuning (SI units). Change these while playing. ----------------------------------------------------------
static const float MASS = 1200.0f;                  // kg
static const float HALF_WIDTH = 0.9f;               // for the inertia tensor
static const float HALF_HEIGHT = 0.5f;
static const float HALF_LENGTH = 2.0f;
static const float AXLE_OFFSET = 1.3f;              // distance of each axle from the centre of mass (wheel base 2.6 m)
static const float GRAVITY = 9.81f;

// Engine / brakes / resistance.
static const float ENGINE_MAX_FORCE = 5500.0f;      // N: traction-control limit at the driven wheels (wheel-spin limit at low speed)
static const float TOP_SPEED = 55.0f;               // m/s: engine cuts out above this
static const float BRAKE_FORCE = 11000.0f;          // N, split between the axles
static const float BRAKE_FRONT_SHARE = 0.65f;
static const float HANDBRAKE_FORCE = 2000.0f;       // N, rear axle only
static const float AIR_DRAG = 0.9f;                 // N per (m/s)^2
static const float ROLLING_RESISTANCE = 180.0f;     // N

// Drivetrain, built like PhysX's vehicle SDK builds it: engine torque curve -> gearbox -> final drive -> wheels,
// with an automatic gearbox that shifts on engine speed. (Tuned so acceleration matches the old flat power curve.)
static const float ENGINE_IDLE_RPM = 1000.0f;
static const float ENGINE_MAX_RPM = 7000.0f;
static const float ENGINE_PEAK_TORQUE = 365.0f;     // N*m
static const float ENGINE_LAUNCH_RPM = 2800.0f;     // clutch slip: from a standstill the engine runs at least this fast on the throttle
static const int   GEAR_COUNT = 6;
static const float GEAR_RATIOS[GEAR_COUNT] = { 3.6f, 2.2f, 1.55f, 1.15f, 0.92f, 0.76f };
static const float FINAL_RATIO = 3.9f;
static const float DRIVETRAIN_EFFICIENCY = 0.9f;
static const float WHEEL_RADIUS = 0.32f;
static const float SHIFT_UP_RPM = 6000.0f;
static const float SHIFT_DOWN_RPM = 2400.0f;
static const float SHIFT_TIME = 0.18f;              // s with no torque while the gear changes
static const float SHIFT_LATENCY = 0.5f;            // s before the next change is allowed (stops hunting between gears)
static const float RPM_RESPONSE = 15.0f;            // 1/s, how quickly the engine speed follows the wheels

// Weight transfer: accelerating loads the rear axle, braking loads the front. That is what makes the rear step out when you
// lift off in a corner and what gives you traction on the throttle. (PhysX gets this from its suspension; here it is computed.)
static const float CG_HEIGHT = 0.5f;                // m
static const float WEIGHT_TRANSFER_SCALE = 0.7f;    // < 1: arcade-friendly, a real car with this height would transfer more
static const float LONG_ACCEL_RESPONSE = 10.0f;     // 1/s, smoothing of the acceleration used for the transfer
static const float LOAD_SENSITIVITY = 0.12f;        // grip per unit load falls a little as the load rises (PhysX's saturating lateral stiffness)

// Tyres. Friction coefficients above 1 are normal for road tyres. MORE GRIP = HIGHER VALUES.
static const float FRICTION_FRONT = 1.3f;           // front a little below the rear: when you ask too much the FRONT slides first (safe understeer)
static const float FRICTION_FRONT_DRIFT = 1.2f;     // front grip while the handbrake is held: the drift is tuned for this value, keep it as is
static const float FRICTION_REAR = 1.5f;
static const float HANDBRAKE_REAR_FRICTION_SCALE = 0.75f;
static const float OFFROAD_FRICTION_SCALE = 0.55f;
static const float OFFROAD_ROLLING_SCALE = 8.0f;
static const float COMBINED_SLIP = 0.35f;          // how much engine / brake force eats into cornering grip (1 = full friction circle, 0 = none)
static const float TYRE_STIFFNESS = 8.0f;           // how sharply grip builds up with slip angle (B)
static const float TYRE_SHAPE = 1.3f;               // >1: grip falls a little after the peak, which is what makes drifting possible (C)
static const float SLIP_SPEED_SMOOTHING = 2.0f;     // m/s, keeps the slip angle well behaved at walking speed

// Steering.
static const float STEER_MAX_ANGLE = 0.5f;          // rad of front wheel angle at walking speed
static const float STEER_LOCK_ACCEL = 14.0f;        // m/s^2: full steering at ANY speed asks for about this much cornering force.
                                                    // Slightly above what the tyres give (~12-13), so full lock = at / just past the limit.
static const float DRIFT_STEER_GAIN = 1.2f;        // extra lock allowed per radian of slide angle
static const float STEER_RATE = 7.0f;               // how quickly the wheel follows the keys (1/s)
static const float STEER_RETURN_RATE = 14.0f;       // ...and how quickly it comes back to centre when you let go (a lazy return makes high speed feel boaty)
static const float STEER_SLIP_ALLOWANCE = 0.018f;   // rad of extra lock at EVERY speed. Real tyres need roughly this much wheel angle beyond the geometric
                                                    // one just to make the sideways force; without it the lock at 40 m/s is slightly too small to hold the
                                                    // tightest bends, the car runs wide and steering feels "dead" at top speed.

// Walls beside the road.
static const float ROAD_SHOULDER = 8.0f;
static const float WALL_RESTITUTION = 0.3f;
static const float WALL_SCRUB = 0.92f;

// Physics runs in fixed steps so tyre forces stay stable whatever the frame rate is.
static const float FIXED_DT = 1.0f / 120.0f;
static const int MAX_STEPS_PER_FRAME = 12;

Car::Car()
{
    Physics::Matrix3 tensor;

    m_transform = std::make_shared<Physics::Transform>();
    m_body = std::make_shared<Physics::RigidBody>(m_transform);

    // The module's mass setters are two calls: SetMass stores the mass, SetInverseMass takes the MASS and inverts it.
    m_body->SetMass(MASS);
    m_body->SetInverseMass(MASS);

    tensor.SetBlockInertiaTensor(v3df(HALF_WIDTH, HALF_HEIGHT, HALF_LENGTH), MASS);
    m_body->SetInertiaTensor(tensor);

    // Damping factor 1 = none (the tyres and air drag below do the slowing down).
    m_body->SetDamping(1.0f, 0.4f);     // yaw damping: stops a slide from turning into a spin
    m_body->SetCanRest(false);

    m_yawRate = 0.0f;
    m_heading = 0.0f;
    m_forwardSpeed = 0.0f;
    m_slipAngle = 0.0f;
    m_steer = 0.0f;
    m_steerAngle = 0.0f;
    m_s = 0.0f;
    m_d = 0.0f;
    m_offRoad = false;
    m_accumulator = 0.0f;
    m_rpm = ENGINE_IDLE_RPM;
    m_gear = 1;
    m_shiftTimer = 0.0f;
    m_gearTimer = 0.0f;
    m_longAccel = 0.0f;
    m_wallHit = false;

    Reset(0.0f, 0.0f, 0.0f);
}

void Car::Reset(float x, float z, float heading, float speed, float s)
{
    // SetRotation also overwrites the orientation's i/j/k, so call it first and set the orientation afterwards.
    m_body->SetRotation(0.0f, 0.0f, 0.0f);
    m_body->SetOrientation(cosf(heading * 0.5f), 0.0f, sinf(heading * 0.5f), 0.0f);
    m_body->SetPosition(x, 0.0f, z);
    m_body->SetVelocity(sinf(heading) * speed, 0.0f, cosf(heading) * speed);
    m_body->ResetForces();
    m_body->CalculateDerivedData();

    m_steer = 0.0f;
    m_steerAngle = 0.0f;
    m_s = s;
    m_d = 0.0f;
    m_offRoad = false;
    m_accumulator = 0.0f;
    m_rpm = ENGINE_IDLE_RPM;
    m_shiftTimer = 0.0f;
    m_gearTimer = 0.0f;
    m_longAccel = 0.0f;
    m_wallHit = false;

    // Start in the lowest gear that keeps the engine under 5000 rpm at the starting speed.
    m_gear = 1;
    while(m_gear < GEAR_COUNT && speed / WHEEL_RADIUS * GEAR_RATIOS[m_gear - 1] * FINAL_RATIO * 9.5493f > 5000.0f)
    {
        m_gear++;
    }

    ReadState();
}

XMFLOAT3 Car::GetPosition() const
{
    v3df position = m_body->GetPosition();

    return XMFLOAT3(position.x, 0.0f, position.z);
}

void Car::Update(float dt, const CarControls& controls, const Road& road)
{
    RoadPoint point;
    float grade;
    int steps = 0;

    // Gravity along the slope of the road where the car is (held for this frame's physics steps).
    point = road.GetPoint(m_s);
    grade = road.GetGrade(m_s);
    m_slopeForce = v3df(sinf(point.heading), 0.0f, cosf(point.heading)) * (-MASS * GRAVITY * grade / sqrtf(1.0f + grade * grade));

    m_accumulator += dt;

    while(m_accumulator >= FIXED_DT && steps < MAX_STEPS_PER_FRAME)
    {
        Step(controls);
        m_accumulator -= FIXED_DT;
        steps++;
    }

    // After a long hitch drop the backlog instead of simulating it.
    if(steps == MAX_STEPS_PER_FRAME)
    {
        m_accumulator = 0.0f;
    }

    ApplyWalls(road);
}

// Reads the body into plain numbers: velocity, nose direction, heading, yaw rate, slip angle.
void Car::ReadState()
{
    m_velocity = m_body->GetVelocity();
    m_velocity.y = 0.0f;

    m_forward = m_body->GetDirectionInWorldSpace(v3df(0.0f, 0.0f, 1.0f));
    m_forward.y = 0.0f;
    m_forward = m_forward.normalize();

    m_yawRate = m_body->GetRotation().y;
    m_heading = atan2f(m_forward.x, m_forward.z);
    m_forwardSpeed = m_velocity.dot(m_forward);

    // Sideways speed in the car's frame; the right-hand side of the nose is (cos h, -sin h).
    float sideways = m_velocity.x * m_forward.z - m_velocity.z * m_forward.x;

    m_slipAngle = atan2f(sideways, fabsf(m_forwardSpeed) + SLIP_SPEED_SMOOTHING);
}

// Engine torque at a given fraction of maximum engine speed (a normalised curve, like PhysX's torque curve).
static float EngineTorqueCurve(float rpmFraction)
{
    static const float points[7][2] = { {0.00f, 0.35f}, {0.14f, 0.55f}, {0.30f, 0.80f}, {0.50f, 1.00f}, {0.70f, 0.97f}, {0.85f, 0.85f}, {1.00f, 0.60f} };
    int i;

    if(rpmFraction <= points[0][0]) { return points[0][1]; }

    for(i=1; i<7; i++)
    {
        if(rpmFraction <= points[i][0])
        {
            float t = (rpmFraction - points[i - 1][0]) / (points[i][0] - points[i - 1][0]);
            return points[i - 1][1] + (points[i][1] - points[i - 1][1]) * t;
        }
    }

    return points[6][1];
}

// Engine speed follows the wheels through the current gear; the automatic gearbox shifts up / down on engine speed;
// the force at the driven wheels is torque * total ratio * efficiency / wheel radius, capped by traction control.
float Car::UpdateDrivetrain(float throttle, float forwardSpeed)
{
    float ratio, wheelRpm, engineRpm, torque, force;

    ratio = GEAR_RATIOS[m_gear - 1] * FINAL_RATIO;
    wheelRpm = fmaxf(forwardSpeed, 0.0f) / WHEEL_RADIUS * ratio * 9.5493f;      // rad/s -> rpm

    // Slipping clutch at a standstill: the engine is allowed to run faster than the wheels would turn it.
    engineRpm = wheelRpm;
    if(throttle > 0.0f && engineRpm < ENGINE_LAUNCH_RPM)
    {
        engineRpm = ENGINE_LAUNCH_RPM;
    }

    engineRpm = fmaxf(ENGINE_IDLE_RPM, fminf(ENGINE_MAX_RPM, engineRpm));
    m_rpm += (engineRpm - m_rpm) * fminf(1.0f, RPM_RESPONSE * FIXED_DT);

    // Automatic gearbox.
    m_gearTimer += FIXED_DT;

    if(m_shiftTimer > 0.0f)
    {
        m_shiftTimer -= FIXED_DT;
    }
    else if(m_gearTimer > SHIFT_LATENCY)
    {
        if(m_rpm > SHIFT_UP_RPM && m_gear < GEAR_COUNT)
        {
            m_gear++;
            m_shiftTimer = SHIFT_TIME;
            m_gearTimer = 0.0f;
        }
        else if(m_rpm < SHIFT_DOWN_RPM && m_gear > 1 && forwardSpeed > 3.0f)
        {
            m_gear--;
            m_shiftTimer = SHIFT_TIME;
            m_gearTimer = 0.0f;
        }
    }

    // No torque while shifting, off the throttle, at the rev limiter or above top speed.
    if(throttle <= 0.0f || m_shiftTimer > 0.0f || m_rpm >= ENGINE_MAX_RPM * 0.98f || forwardSpeed >= TOP_SPEED)
    {
        return 0.0f;
    }

    torque = throttle * ENGINE_PEAK_TORQUE * EngineTorqueCurve(m_rpm / ENGINE_MAX_RPM);
    force = torque * ratio * DRIVETRAIN_EFFICIENCY / WHEEL_RADIUS;

    return fminf(force, ENGINE_MAX_FORCE);
}

// Linear bicycle-model analysis of the tyre settings (Riekert & Schunck, the model in the bicycle_model reference):
// cornering stiffness c = d(force)/d(slip angle) at zero slip = shape * stiffness * friction * load.
// Understeer gradient K = (m / L) * (b / c_front - a / c_rear) with a = b = the axle distances from the centre of mass.
// Characteristic speed = sqrt(L / K): the speed at which the steering response is twice that of a neutral car.
void Car::GetHandlingBalance(float& understeerGradient, float& characteristicSpeed)
{
    float load = MASS * GRAVITY * 0.5f;
    float cFront = TYRE_SHAPE * TYRE_STIFFNESS * FRICTION_FRONT * load;
    float cRear = TYRE_SHAPE * TYRE_STIFFNESS * FRICTION_REAR * load;
    float wheelBase = 2.0f * AXLE_OFFSET;

    understeerGradient = (MASS / wheelBase) * (AXLE_OFFSET / cFront - AXLE_OFFSET / cRear);
    characteristicSpeed = (understeerGradient > 1e-9f) ? sqrtf(wheelBase / understeerGradient) : 1e9f;
}

void Car::Step(const CarControls& controls)
{
    float blend, lock, speed, driveForce, brakeForce, handbrakeForce, surface, frontFriction, rearFriction, brakeSign, rolling;
    float staticLoad, transfer, frontLoad, rearLoad;
    v3df drag;

    ReadState();

    speed = m_velocity.magnitude();

    // Where the car is decides the surface (the previous frame's value is plenty accurate).
    surface = m_offRoad ? OFFROAD_FRICTION_SCALE : 1.0f;

    // 1. Steering wheel eases toward the keys, front wheel angle shrinks with speed.
    blend = ((fabsf(controls.steer) < fabsf(m_steer)) ? STEER_RETURN_RATE : STEER_RATE) * FIXED_DT;
    m_steer += (controls.steer - m_steer) * blend;
    // A fixed wheel angle would mean a hugely different cornering force at 10 and at 40 m/s (force ~ angle * speed^2),
    // so the lock shrinks with speed squared: the keys then feel the same at every speed.
    lock = STEER_LOCK_ACCEL * (2.0f * AXLE_OFFSET) / fmaxf(speed * speed, 1.0f) + STEER_SLIP_ALLOWANCE * (1.0f - controls.handbrake);

    // While the car is sliding, allow the wheels to turn as far as the slide angle: that is what lets you
    // counter-steer (point the front wheels where the car is actually going) and hold or catch a drift.
    lock = fmaxf(lock, DRIFT_STEER_GAIN * fabsf(m_slipAngle));
    m_steerAngle = m_steer * fminf(STEER_MAX_ANGLE, lock);

    // 2. Engine -> gearbox -> wheels: pushes the rear axle.
    driveForce = UpdateDrivetrain(controls.throttle, m_forwardSpeed);

    // 3. Brakes oppose the motion of the wheel; the clamp stops them from jittering around standstill.
    brakeSign = -fmaxf(-1.0f, fminf(1.0f, m_forwardSpeed));
    brakeForce = BRAKE_FORCE * controls.brake * brakeSign;
    handbrakeForce = HANDBRAKE_FORCE * controls.handbrake * brakeSign;

    frontFriction = (FRICTION_FRONT + (FRICTION_FRONT_DRIFT - FRICTION_FRONT) * controls.handbrake) * surface;
    rearFriction = FRICTION_REAR * surface;
    if(controls.handbrake > 0.0f)
    {
        rearFriction *= HANDBRAKE_REAR_FRICTION_SCALE;      // locked rear wheels grip far less sideways
    }

    // 4. Weight transfer. At rest each axle carries half the weight; accelerating moves load to the rear axle, braking to the
    //    front (transfer = m * a * h / wheelbase). More load = more grip force, but a bit less grip per unit of load.
    m_longAccel += (m_body->GetLastFrameAcceleration().dot(m_forward) - m_longAccel) * fminf(1.0f, LONG_ACCEL_RESPONSE * FIXED_DT);

    staticLoad = MASS * GRAVITY * 0.5f;
    transfer = MASS * m_longAccel * CG_HEIGHT / (2.0f * AXLE_OFFSET) * WEIGHT_TRANSFER_SCALE;
    frontLoad = fmaxf(0.25f * staticLoad, staticLoad - transfer);
    rearLoad = fmaxf(0.25f * staticLoad, staticLoad + transfer);

    frontFriction *= powf(frontLoad / staticLoad, -LOAD_SENSITIVITY);
    rearFriction *= powf(rearLoad / staticLoad, -LOAD_SENSITIVITY);

    // 5. Each axle turns its slip into a force.
    ApplyTyre(AXLE_OFFSET, m_heading + m_steerAngle, brakeForce * BRAKE_FRONT_SHARE, frontFriction, frontLoad);
    ApplyTyre(-AXLE_OFFSET, m_heading, driveForce + brakeForce * (1.0f - BRAKE_FRONT_SHARE) + handbrakeForce, rearFriction, rearLoad);

    // 6. Air drag and rolling resistance (grass is much stickier than tarmac).
    rolling = ROLLING_RESISTANCE * (m_offRoad ? OFFROAD_ROLLING_SCALE : 1.0f);
    drag = m_velocity * (-AIR_DRAG * speed);
    if(speed > 0.01f)
    {
        drag += m_velocity.normalize() * (-rolling * fminf(1.0f, speed));
    }

    m_body->AddForce(drag);
    m_body->AddForce(m_slopeForce);

    // 7. Let the Physics module integrate force -> acceleration -> velocity -> position / orientation.
    m_body->Update(FIXED_DT);

    // The ground is flat and the forces are all horizontal, so keep the body on the plane.
    v3df position = m_body->GetPosition();
    v3df velocity = m_body->GetVelocity();
    m_body->SetPosition(position.x, 0.0f, position.z);
    m_body->SetVelocity(velocity.x, 0.0f, velocity.z);
}

// One tyre: finds how the contact point is sliding and turns it into a force at that point.
// requestedForce is the engine / brake force along the wheel, which uses up part of the available grip.
void Car::ApplyTyre(float offsetZ, float wheelAngle, float requestedForce, float friction, float load)
{
    v3df offset, pointVelocity, wheelForward, wheelRight, force;
    float velocityAlong, velocitySide, slip, maxForce, along, sideCapacity, side, stableLimit, torque;

    // Contact point relative to the centre of mass, and its velocity: v + w x r (w is about the Y axis).
    offset = m_forward * offsetZ;
    pointVelocity = v3df(m_velocity.x + m_yawRate * offset.z, 0.0f, m_velocity.z - m_yawRate * offset.x);

    wheelForward = v3df(sinf(wheelAngle), 0.0f, cosf(wheelAngle));
    wheelRight = v3df(cosf(wheelAngle), 0.0f, -sinf(wheelAngle));

    velocityAlong = pointVelocity.dot(wheelForward);
    velocitySide = pointVelocity.dot(wheelRight);

    // Friction circle: grip is a budget. Engine / brake force along the wheel is paid first, what is left is for cornering.
    maxForce = friction * load;

    along = requestedForce;
    if(along > maxForce)  { along = maxForce; }
    if(along < -maxForce) { along = -maxForce; }

    // A softened friction circle: pure engine force costs some cornering grip, but never nearly all of it
    // (the full circle makes a rear-wheel-drive car spin as soon as you steer on the throttle).
    sideCapacity = maxForce * sqrtf(fmaxf(0.0f, 1.0f - COMBINED_SLIP * (along / maxForce) * (along / maxForce)));

    // Slip angle -> sideways force. Rises quickly, peaks, then falls a little: past the peak the tyre is sliding.
    slip = atan2f(velocitySide, fabsf(velocityAlong) + SLIP_SPEED_SMOOTHING);
    side = -sideCapacity * sinf(TYRE_SHAPE * atanf(TYRE_STIFFNESS * slip));

    // Never ask for more sideways force than it takes to stop the sideways motion within one step
    // (otherwise the force overshoots and buzzes at low speed).
    stableLimit = (MASS * 0.5f) * fabsf(velocitySide) / FIXED_DT;
    if(side > stableLimit)  { side = stableLimit; }
    if(side < -stableLimit) { side = -stableLimit; }

    force = wheelForward * along + wheelRight * side;

    // Force at the centre of mass, plus the torque it makes around the Y axis: (r x F).y = r.z * F.x - r.x * F.z
    // (The module's AddForceAtPoint is not used: it adds the torque term to the force.)
    torque = offset.z * force.x - offset.x * force.z;

    m_body->AddForce(force);
    m_body->AddTorque(v3df(0.0f, torque, 0.0f));
}

// Finds where the car is on the road and keeps it between the walls.
void Car::ApplyWalls(const Road& road)
{
    RoadProjection projection;
    RoadPoint center;
    v3df position, velocity, right;
    float limit, penetration, normalSpeed;

    position = m_body->GetPosition();
    projection = road.Project(position.x, position.z, m_s);

    m_s = projection.s;
    m_d = projection.d;
    m_offRoad = fabsf(m_d) > ROAD_HALF_WIDTH;

    limit = ROAD_HALF_WIDTH + ROAD_SHOULDER;

    if(fabsf(m_d) <= limit)
    {
        return;
    }

    center = road.GetPoint(m_s);
    right = v3df(cosf(center.heading), 0.0f, -sinf(center.heading));

    // Push the car back to the wall...
    penetration = (m_d > 0.0f) ? (m_d - limit) : (m_d + limit);
    m_body->SetPosition(position.x - right.x * penetration, 0.0f, position.z - right.z * penetration);
    m_d = (m_d > 0.0f) ? limit : -limit;

    // ...and bounce the velocity off it, losing some speed.
    velocity = m_body->GetVelocity();
    normalSpeed = velocity.dot(right);

    if((m_d > 0.0f && normalSpeed > 0.0f) || (m_d < 0.0f && normalSpeed < 0.0f))
    {
        velocity.AddScaledVector(right, -(1.0f + WALL_RESTITUTION) * normalSpeed);
        m_wallHit = true;
        velocity *= WALL_SCRUB;
        m_body->SetVelocity(velocity);
    }

    m_body->CalculateDerivedData();
}

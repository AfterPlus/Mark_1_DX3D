#include "Fence.h"

using Physics::v3df;

// ---- Posts ---------------------------------------------------------------------------------------------------
static const float POST_SPACING = 18.0f;
static const float POST_OFFSET = ROAD_HALF_WIDTH + 1.2f;
static const float POST_HALF_WIDTH = 0.15f;         // cube.txt is 2 x 2 x 2: the post is 0.3 x 1.4 x 0.3 m
static const float POST_HALF_HEIGHT = 0.7f;
static const float POST_MASS = 30.0f;               // kg, a wooden post
static const float GRAVITY = 9.81f;

// ---- The hit -------------------------------------------------------------------------------------------------
static const float HIT_RESTITUTION = 0.4f;          // how much of the closing speed bounces back (0 = the post sticks to the bumper)
static const float HIT_FRICTION = 0.5f;             // bumper against wood: a car sliding sideways drags the post along and spins it
static const float HIT_MIN_SPEED = 0.5f;            // m/s: slower than this the car only leans on the post
static const float HIT_LIFT = 0.35f;                // upward tilt of the contact normal: the nose / flank of the car is rounded, so it
                                                    // scoops the post up instead of pushing it flat along the ground
static const float BUMPER_HEIGHT_SHARE = 0.3f;      // where on the car's height the post is struck (low: the bumper)

// ---- Loose posts ---------------------------------------------------------------------------------------------
static const float LINEAR_DAMPING = 0.9f;           // velocity kept per second (air drag)
static const float ANGULAR_DAMPING = 0.6f;
static const float GROUND_RESTITUTION = 0.3f;
static const float GROUND_BOUNCE_SPEED = 1.5f;      // m/s: slower impacts do not bounce (stops the post buzzing on the ground)
static const float GROUND_FRICTION = 0.6f;
static const int   CONTACT_ITERATIONS = 4;
static const float REST_SPEED = 0.2f;               // below this speed / spin for REST_TIME the post goes to sleep
static const float REST_SPIN = 0.4f;
static const float REST_TIME = 0.5f;
static const float MAX_AGE = 15.0f;                 // s: a post that is still rocking on the ground after this long is put to sleep
static const int   MAX_LOOSE = 32;                  // oldest loose post is dropped beyond this

static const float FIXED_DT = 1.0f / 120.0f;
static const int   MAX_STEPS_PER_FRAME = 12;

// Foot of the standing post `index` on `side` (-1 left, +1 right).
static XMFLOAT3 PostBase(const Road& road, int index, int side)
{
    float s = (float)index * POST_SPACING;
    float d = (float)side * POST_OFFSET;
    RoadPoint point = road.GetPoint(s);

    return XMFLOAT3(point.x + cosf(point.heading) * d, road.GetSurfaceHeight(s, d), point.z - sinf(point.heading) * d);
}

// 1 / effective mass of a body for an impulse along n at offset r from its centre of mass: 1/m + n . ((I^-1 (r x n)) x r).
static float InverseMassAt(const Physics::RigidBody& body, const v3df& r, const v3df& n)
{
    v3df angular = body.GetInverseInertiaTensorWorld().Transform(r % n);

    return body.GetInverseMass() + (angular % r).dot(n);
}

// An instant change of momentum at offset r: velocity += J/m, angular velocity += I^-1 (r x J).
static void ApplyImpulse(Physics::RigidBody& body, const v3df& impulse, const v3df& r)
{
    v3df velocity = body.GetVelocity();
    v3df rotation = body.GetRotation();

    velocity.AddScaledVector(impulse, body.GetInverseMass());
    rotation += body.GetInverseInertiaTensorWorld().Transform(r % impulse);

    body.SetVelocity(velocity);
    body.SetRotation(rotation);
}

static float Clamp(float value, float low, float high)
{
    return (value < low) ? low : ((value > high) ? high : value);
}

Fence::Fence()
{
    m_accumulator = 0.0f;
}

void Fence::Reset()
{
    m_knocked.clear();
    m_loose.clear();
    m_accumulator = 0.0f;
}

bool Fence::IsKnocked(int index, int side) const
{
    int key = index * 2 + (side > 0 ? 1 : 0);

    for(size_t i=0; i<m_knocked.size(); i++)
    {
        if(m_knocked[i] == key)
        {
            return true;
        }
    }

    return false;
}

bool Fence::Update(float dt, Car& car, const Road& road, float carHalfWidth, float carHalfLength, float carHeight, float carBaseY, float minS)
{
    int index, first, last, side, steps;
    float reach;
    bool hit = false;

    // Posts near the car (its corners can reach half its diagonal away).
    reach = sqrtf(carHalfWidth * carHalfWidth + carHalfLength * carHalfLength) + POST_OFFSET + POST_HALF_WIDTH;
    first = (int)floorf((car.GetS() - reach) / POST_SPACING);
    last = (int)ceilf((car.GetS() + reach) / POST_SPACING);

    for(index=first; index<=last; index++)
    {
        if((float)index * POST_SPACING < road.GetMinS())
        {
            continue;
        }

        for(side=-1; side<=1; side+=2)
        {
            if(!IsKnocked(index, side) && HitPost(car, road, index, side, carHalfWidth, carHalfLength, carHeight, carBaseY))
            {
                hit = true;
            }
        }
    }

    // Loose posts: fixed steps, like the car, so the contacts behave the same at any frame rate.
    m_accumulator += dt;
    steps = 0;

    while(m_accumulator >= FIXED_DT && steps < MAX_STEPS_PER_FRAME)
    {
        for(size_t i=0; i<m_loose.size(); i++)
        {
            if(!m_loose[i].resting)
            {
                StepPost(m_loose[i], road);
            }
        }

        m_accumulator -= FIXED_DT;
        steps++;
    }

    if(steps == MAX_STEPS_PER_FRAME)
    {
        m_accumulator = 0.0f;
    }

    // Forget what the car has left behind (and the oldest loose posts if there are too many).
    for(size_t i=m_loose.size(); i-- > 0; )
    {
        if(m_loose[i].s < minS)
        {
            m_loose.erase(m_loose.begin() + i);
        }
    }

    while((int)m_loose.size() > MAX_LOOSE)
    {
        m_loose.erase(m_loose.begin());
    }

    for(size_t i=m_knocked.size(); i-- > 0; )
    {
        if((float)(m_knocked[i] / 2) * POST_SPACING < minS - POST_SPACING)
        {
            m_knocked.erase(m_knocked.begin() + i);
        }
    }

    return hit;
}

// Car (a box on the ground, as drawn) against one standing post (a thin upright box). On contact the post breaks off and the
// two exchange an impulse at the contact point: J = -(1 + e) v_closing / (k_post + k_car), plus friction along the slide.
bool Fence::HitPost(Car& car, const Road& road, int index, int side, float carHalfWidth, float carHalfLength, float carHeight, float carBaseY)
{
    XMFLOAT3 base, carPosition, carVelocity, point, horizontal;
    float heading, forwardX, forwardZ, relX, relZ, localX, localZ, closestX, closestZ, deltaX, deltaZ, distance;
    float normalX, normalZ, worldNormalX, worldNormalZ, contactX, contactZ, yawRate, approach, closing, impulse, tangentSpeed;
    float kPost, kCar, horizontalShare;
    v3df normal, r, carPointVelocity, relative, tangent, total;
    Physics::Matrix3 tensor;
    LoosePost post;

    base = PostBase(road, index, side);

    // The car's body and the post must overlap in height (a post down a bank or up on one can be missed).
    if(base.y > carBaseY + carHeight || base.y + 2.0f * POST_HALF_HEIGHT < carBaseY)
    {
        return false;
    }

    // Post centre in the car's frame: x to the right, z along the nose.
    carPosition = car.GetPosition();
    heading = car.GetHeading();
    forwardX = sinf(heading);
    forwardZ = cosf(heading);
    relX = base.x - carPosition.x;
    relZ = base.z - carPosition.z;
    localX = relX * forwardZ - relZ * forwardX;
    localZ = relX * forwardX + relZ * forwardZ;

    // Closest point of the car's rectangle, and the normal from the car toward the post.
    closestX = Clamp(localX, -carHalfWidth, carHalfWidth);
    closestZ = Clamp(localZ, -carHalfLength, carHalfLength);
    deltaX = localX - closestX;
    deltaZ = localZ - closestZ;
    distance = sqrtf(deltaX * deltaX + deltaZ * deltaZ);

    if(distance > 0.0f)
    {
        if(distance >= POST_HALF_WIDTH)
        {
            return false;
        }

        normalX = deltaX / distance;
        normalZ = deltaZ / distance;
    }
    else if(carHalfWidth - fabsf(localX) < carHalfLength - fabsf(localZ))
    {
        // Post centre inside the car (a fast frame): push it out through the nearest face.
        normalX = (localX >= 0.0f) ? 1.0f : -1.0f;
        normalZ = 0.0f;
        closestX = normalX * carHalfWidth;
    }
    else
    {
        normalX = 0.0f;
        normalZ = (localZ >= 0.0f) ? 1.0f : -1.0f;
        closestZ = normalZ * carHalfLength;
    }

    // Back to the world: right = (cos h, -sin h) = (forwardZ, -forwardX).
    worldNormalX = normalX * forwardZ + normalZ * forwardX;
    worldNormalZ = -normalX * forwardX + normalZ * forwardZ;
    contactX = carPosition.x + closestX * forwardZ + closestZ * forwardX;
    contactZ = carPosition.z - closestX * forwardX + closestZ * forwardZ;
    point = XMFLOAT3(contactX, Clamp(carBaseY + carHeight * BUMPER_HEIGHT_SHARE, base.y + 0.05f, base.y + 2.0f * POST_HALF_HEIGHT - 0.05f), contactZ);

    // Velocity of the car's surface at the contact: v + w x r (w about +Y).
    carVelocity = car.GetVelocity();
    yawRate = car.GetYawRate();
    carPointVelocity = v3df(carVelocity.x + yawRate * (contactZ - carPosition.z), 0.0f, carVelocity.z - yawRate * (contactX - carPosition.x));

    approach = carPointVelocity.x * worldNormalX + carPointVelocity.z * worldNormalZ;
    if(approach < HIT_MIN_SPEED)
    {
        return false;
    }

    // The post breaks off: a rigid body at rest, standing where it stood.
    post.transform = std::make_shared<Physics::Transform>();
    post.body = std::make_shared<Physics::RigidBody>(post.transform);
    post.s = (float)index * POST_SPACING;
    post.age = 0.0f;
    post.stillTime = 0.0f;
    post.resting = false;

    Physics::RigidBody& body = *post.body;
    float postHeading = road.GetPoint(post.s).heading;

    body.SetMass(POST_MASS);
    tensor.SetBlockInertiaTensor(v3df(POST_HALF_WIDTH, POST_HALF_HEIGHT, POST_HALF_WIDTH), POST_MASS);
    body.SetInertiaTensor(tensor);
    body.SetDamping(LINEAR_DAMPING, ANGULAR_DAMPING);
    body.SetCanRest(false);
    body.SetOrientation(cosf(postHeading * 0.5f), 0.0f, sinf(postHeading * 0.5f), 0.0f);
    body.SetPosition(base.x, base.y + POST_HALF_HEIGHT, base.z);
    body.SetVelocity(0.0f, 0.0f, 0.0f);
    body.SetRotation(0.0f, 0.0f, 0.0f);
    body.ResetForces();
    body.CalculateDerivedData();

    // Contact normal tilted up a little (the car scoops the post up), offset of the contact from the post's centre.
    normal = v3df(worldNormalX, HIT_LIFT, worldNormalZ).normalize();
    r = v3df(point.x, point.y, point.z) - body.GetPosition();
    horizontalShare = worldNormalX * normal.x + worldNormalZ * normal.z;
    horizontal = XMFLOAT3(worldNormalX, 0.0f, worldNormalZ);

    // Normal impulse. The car only takes the horizontal part (the ground under its tyres takes the vertical part).
    relative = v3df(-carPointVelocity.x, 0.0f, -carPointVelocity.z);    // post (at rest) relative to the car
    closing = relative.dot(normal);
    kPost = InverseMassAt(body, r, normal);
    kCar = car.GetInverseMassAt(point, horizontal) * horizontalShare * horizontalShare;
    impulse = -(1.0f + HIT_RESTITUTION) * closing / (kPost + kCar);
    total = normal * impulse;

    // Friction: the part of the car's motion along the contact surface drags the post with it (Coulomb: at most mu * J).
    tangent = relative - normal * closing;
    tangentSpeed = tangent.magnitude();
    if(tangentSpeed > 0.01f)
    {
        tangent = tangent * (1.0f / tangentSpeed);
        float friction = fminf(HIT_FRICTION * impulse, tangentSpeed / (InverseMassAt(body, r, tangent) + kCar));
        total.AddScaledVector(tangent, -friction);
    }

    ApplyImpulse(body, total, r);
    car.ApplyImpulse(XMFLOAT3(-total.x, 0.0f, -total.z), point);

    m_knocked.push_back(index * 2 + (side > 0 ? 1 : 0));
    m_loose.push_back(post);

    return true;
}

// One fixed step of a loose post: gravity and air drag through the Physics module, then impulse contacts between its eight
// corners and the ground (with bounce and friction), then a push out of the ground.
void Fence::StepPost(LoosePost& post, const Road& road)
{
    static const float corners[8][3] = { {-1,-1,-1}, {1,-1,-1}, {-1,1,-1}, {1,1,-1}, {-1,-1,1}, {1,-1,1}, {-1,1,1}, {1,1,1} };
    Physics::RigidBody& body = *post.body;
    RoadProjection projection;
    RoadPoint roadPoint;
    v3df position, up, forward, right, normal, groundPoint, world[8], r, velocity, tangent;
    float height, gradeS, gradeD, depth, maxDepth, closing, restitution, impulse, speed, friction;
    bool touching;
    int i, iteration;

    body.AddForce(v3df(0.0f, -POST_MASS * GRAVITY, 0.0f));
    body.Update(FIXED_DT);
    post.age += FIXED_DT;

    // The ground under the post: its height and its slope (banks, drops and hills beside the road).
    position = body.GetPosition();
    projection = road.Project(position.x, position.z, post.s);
    post.s = projection.s;

    height = road.GetSurfaceHeight(projection.s, projection.d);
    gradeS = road.GetSurfaceHeight(projection.s + 0.5f, projection.d) - road.GetSurfaceHeight(projection.s - 0.5f, projection.d);
    gradeD = road.GetSurfaceHeight(projection.s, projection.d + 0.5f) - road.GetSurfaceHeight(projection.s, projection.d - 0.5f);

    roadPoint = road.GetPoint(projection.s);
    up = v3df(0.0f, 1.0f, 0.0f);
    forward = v3df(sinf(roadPoint.heading), 0.0f, cosf(roadPoint.heading));
    right = v3df(cosf(roadPoint.heading), 0.0f, -sinf(roadPoint.heading));
    normal = (up - forward * gradeS - right * gradeD).normalize();
    groundPoint = v3df(position.x, height, position.z);

    for(i=0; i<8; i++)
    {
        world[i] = body.GetPointInWorldSpace(v3df(corners[i][0] * POST_HALF_WIDTH, corners[i][1] * POST_HALF_HEIGHT, corners[i][2] * POST_HALF_WIDTH));
    }

    touching = false;
    maxDepth = 0.0f;

    for(iteration=0; iteration<CONTACT_ITERATIONS; iteration++)
    {
        for(i=0; i<8; i++)
        {
            depth = (groundPoint - world[i]).dot(normal);
            if(depth <= 0.0f)
            {
                continue;
            }

            touching = true;
            maxDepth = fmaxf(maxDepth, depth);

            // Velocity of this corner: v + w x r. Only corners moving into the ground get an impulse.
            r = world[i] - position;
            velocity = body.GetVelocity() + body.GetRotation() % r;
            closing = velocity.dot(normal);
            if(closing >= 0.0f)
            {
                continue;
            }

            restitution = (closing < -GROUND_BOUNCE_SPEED) ? GROUND_RESTITUTION : 0.0f;
            impulse = -(1.0f + restitution) * closing / InverseMassAt(body, r, normal);
            ApplyImpulse(body, normal * impulse, r);

            // Friction against the sliding direction, at most mu * the normal impulse.
            velocity = body.GetVelocity() + body.GetRotation() % r;
            tangent = velocity - normal * velocity.dot(normal);
            speed = tangent.magnitude();
            if(speed > 1e-4f)
            {
                tangent = tangent * (1.0f / speed);
                friction = fminf(GROUND_FRICTION * impulse, speed / InverseMassAt(body, r, tangent));
                ApplyImpulse(body, tangent * -friction, r);
            }
        }
    }

    // Out of the ground.
    if(maxDepth > 0.0f)
    {
        position.AddScaledVector(normal, maxDepth);
        body.SetPosition(position);
        body.CalculateDerivedData();
    }

    // Lying still: go to sleep.
    if(touching && body.GetVelocity().magnitude() < REST_SPEED && body.GetRotation().magnitude() < REST_SPIN)
    {
        post.stillTime += FIXED_DT;
    }
    else
    {
        post.stillTime = 0.0f;
    }

    if(post.stillTime > REST_TIME || (touching && post.age > MAX_AGE))
    {
        post.resting = true;
        body.SetVelocity(0.0f, 0.0f, 0.0f);
        body.SetRotation(0.0f, 0.0f, 0.0f);
    }
}

void Fence::AddDrawItems(std::vector<RenderItem>& items, ModelClass* model, const Road& road, float meshStart, float meshEnd) const
{
    RenderItem item;
    XMFLOAT3 base;
    XMMATRIX world;
    int index, side;

    item.model = model;

    // Standing posts on both sides, at fixed distances along the road.
    for(index=(int)ceilf(meshStart / POST_SPACING); (float)index * POST_SPACING < meshEnd; index++)
    {
        for(side=-1; side<=1; side+=2)
        {
            if(IsKnocked(index, side))
            {
                continue;
            }

            base = PostBase(road, index, side);

            world = XMMatrixScaling(POST_HALF_WIDTH, POST_HALF_HEIGHT, POST_HALF_WIDTH) *
                    XMMatrixRotationY(road.GetPoint((float)index * POST_SPACING).heading) *
                    XMMatrixTranslation(base.x, base.y + POST_HALF_HEIGHT, base.z);

            XMStoreFloat4x4(&item.world, world);
            items.push_back(item);
        }
    }

    // Loose posts wherever the physics has them.
    for(size_t i=0; i<m_loose.size(); i++)
    {
        Physics::Quaternion orientation = m_loose[i].body->GetOrientation();
        v3df position = m_loose[i].body->GetPosition();

        world = XMMatrixScaling(POST_HALF_WIDTH, POST_HALF_HEIGHT, POST_HALF_WIDTH) *
                XMMatrixRotationQuaternion(XMVectorSet(orientation.i, orientation.j, orientation.k, orientation.r)) *
                XMMatrixTranslation(position.x, position.y, position.z);

        XMStoreFloat4x4(&item.world, world);
        items.push_back(item);
    }
}

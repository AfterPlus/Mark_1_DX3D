#pragma once

#include <vector>
#include <memory>
#include <directxmath.h>
#include "Physics/PhysicsFramework.h"
#include "RendererClass.h"
#include "Road.h"
#include "Car.h"

using namespace DirectX;

// The fence posts beside the road. A post stands still until the car hits it; then it breaks off and becomes a
// Physics::RigidBody: the hit is an impulse exchange between the car and the post (the post flies off the bumper and spins,
// the car loses a little speed and yaws), after which the post falls under gravity, bounces and slides on the ground with
// impulse contacts at its corners, and finally comes to rest.
//
// Standing posts are not stored: post `index` on side -1 / +1 stands at s = index * POST_SPACING. Only the knocked ones are.
class Fence
{
public:
    Fence();

    void Reset();

    // car*: the car as it is drawn (the player sees the posts hit the body, not the physics box); carBaseY = the road height
    // under the car. Posts and knocked-post records behind minS are forgotten. Returns true if the car hit a post.
    bool Update(float dt, Car& car, const Road& road, float carHalfWidth, float carHalfLength, float carHeight, float carBaseY, float minS);

    // Standing posts in [meshStart, meshEnd) and all loose ones.
    void AddDrawItems(std::vector<RenderItem>& items, ModelClass* model, const Road& road, float meshStart, float meshEnd) const;

    int GetLooseCount() const { return (int)m_loose.size(); }

private:
    struct LoosePost
    {
        std::shared_ptr<Physics::Transform> transform;
        std::shared_ptr<Physics::RigidBody> body;
        float s;            // road distance near the post (search hint for Road::Project)
        float age;
        float stillTime;    // how long it has been lying almost still
        bool resting;       // asleep on the ground: no longer simulated
    };

    bool IsKnocked(int index, int side) const;
    bool HitPost(Car& car, const Road& road, int index, int side, float carHalfWidth, float carHalfLength, float carHeight, float carBaseY);
    void StepPost(LoosePost& post, const Road& road);

private:
    std::vector<int> m_knocked;         // index * 2 + (side > 0)
    std::vector<LoosePost> m_loose;
    float m_accumulator;                // leftover time for the fixed-step simulation of the loose posts
};

#pragma once

#include <deque>
#include <math.h>

// The road is a centre line made of points ROAD_STEP metres apart. It is generated on demand in front of the
// car and forgotten behind it, so it is endless. Pure data - no Direct3D in here.
const float ROAD_STEP = 2.0f;
const float ROAD_HALF_WIDTH = 7.0f;

// Conventions (same as the rest of the game): ground is the XZ plane, heading 0 faces +Z,
// positive heading / curvature turns toward +X (right). The right-hand normal is (cos h, 0, -sin h).
struct RoadPoint
{
    float x, z;
    float heading;      // radians, direction of the road at this point
    float curvature;    // 1 / radius. > 0 = bending right, < 0 = bending left
};

// Where a world position lies relative to the road: s = distance along it, d = sideways offset (+ = right).
struct RoadProjection
{
    float s;
    float d;
};

class Road
{
public:
    Road();

    void Reset(unsigned int seed);

    // Makes sure the road exists up to ~480 m in front of carS and drops what is far behind it.
    void Update(float carS);

    // Interpolated point at distance s along the road (clamped to what currently exists).
    RoadPoint GetPoint(float s) const;

    // Finds the closest point on the centre line near hintS (it only searches +-60 m around the hint).
    RoadProjection Project(float x, float z, float hintS) const;

    float GetMinS() const;
    float GetMaxS() const;

private:
    void AppendPoint();
    float NextRandom();     // 0..1

private:
    std::deque<RoadPoint> m_points;
    int m_firstIndex;               // global index of m_points[0]; its distance is m_firstIndex * ROAD_STEP
    float m_targetCurvature;
    float m_segmentRemaining;       // metres until a new bend is chosen
    float m_sign;
    unsigned int m_rng;
};

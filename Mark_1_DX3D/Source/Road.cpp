#include "Road.h"

// Tuning.
static const float MAX_CURVATURE = 0.0065f;     // tightest bend: radius ~150 m
static const float MIN_BEND_FRACTION = 0.3f;    // a bend is never flatter than 30% of the maximum -> no long straights
static const float SEGMENT_MIN = 90.0f;         // metres a bend is held before the next one is chosen
static const float SEGMENT_MAX = 220.0f;
static const float CURVATURE_RATE = 0.00010f;   // how fast the curvature may change, per metre driven
static const float START_STRAIGHT = 80.0f;      // a short straight at the very start so the player can get going
static const float KEEP_BEHIND = 100.0f;
static const float KEEP_AHEAD = 480.0f;
static const int   START_INDEX = -30;           // the road begins 60 m behind the start line (s = 0)

Road::Road()
{
    m_firstIndex = 0;
    m_targetCurvature = 0.0f;
    m_segmentRemaining = 0.0f;
    m_sign = 1.0f;
    m_rng = 1;
}

void Road::Reset(unsigned int seed)
{
    RoadPoint first;

    m_rng = (seed == 0) ? 1 : seed;
    m_points.clear();
    m_firstIndex = START_INDEX;
    m_targetCurvature = 0.0f;
    m_segmentRemaining = START_STRAIGHT;
    m_sign = 1.0f;

    first.x = 0.0f;
    first.z = 0.0f;
    first.heading = 0.0f;
    first.curvature = 0.0f;
    m_points.push_back(first);

    while(GetMaxS() < KEEP_AHEAD)
    {
        AppendPoint();
    }
}

void Road::Update(float carS)
{
    while(GetMaxS() < carS + KEEP_AHEAD)
    {
        AppendPoint();
    }

    // Forget the road far behind the car (always keep at least two points to interpolate between).
    while(m_points.size() > 2 && (float)(m_firstIndex + 1) * ROAD_STEP < carS - KEEP_BEHIND)
    {
        m_points.pop_front();
        m_firstIndex++;
    }
}

float Road::GetMinS() const
{
    return (float)m_firstIndex * ROAD_STEP;
}

float Road::GetMaxS() const
{
    return (float)(m_firstIndex + (int)m_points.size() - 1) * ROAD_STEP;
}

RoadPoint Road::GetPoint(float s) const
{
    RoadPoint result;
    float f, floorF, t;
    int i;

    f = s / ROAD_STEP;
    floorF = floorf(f);
    t = f - floorF;
    i = (int)floorF - m_firstIndex;

    // Clamp to the part of the road that exists.
    if(i < 0)
    {
        i = 0;
        t = 0.0f;
    }

    if(i > (int)m_points.size() - 2)
    {
        i = (int)m_points.size() - 2;
        t = 1.0f;
    }

    const RoadPoint& a = m_points[i];
    const RoadPoint& b = m_points[i + 1];

    result.x = a.x + (b.x - a.x) * t;
    result.z = a.z + (b.z - a.z) * t;
    result.heading = a.heading + (b.heading - a.heading) * t;
    result.curvature = a.curvature + (b.curvature - a.curvature) * t;

    return result;
}

RoadProjection Road::Project(float x, float z, float hintS) const
{
    RoadProjection result;
    float bestDistance, abx, abz, length2, t, cx, cz, dx, dz, heading;
    int i, first, last, count;

    result.s = hintS;
    result.d = 0.0f;
    bestDistance = 1e30f;
    count = (int)m_points.size();

    first = (int)floorf((hintS - 60.0f) / ROAD_STEP) - m_firstIndex;
    last = (int)ceilf((hintS + 60.0f) / ROAD_STEP) - m_firstIndex;

    if(first < 0)         { first = 0; }
    if(last > count - 2)  { last = count - 2; }

    for(i=first; i<=last; i++)
    {
        const RoadPoint& a = m_points[i];
        const RoadPoint& b = m_points[i + 1];

        abx = b.x - a.x;
        abz = b.z - a.z;
        length2 = abx * abx + abz * abz;

        t = ((x - a.x) * abx + (z - a.z) * abz) / length2;
        if(t < 0.0f) { t = 0.0f; }
        if(t > 1.0f) { t = 1.0f; }

        cx = a.x + abx * t;
        cz = a.z + abz * t;
        dx = x - cx;
        dz = z - cz;

        if(dx * dx + dz * dz < bestDistance)
        {
            bestDistance = dx * dx + dz * dz;
            heading = a.heading + (b.heading - a.heading) * t;

            result.s = ((float)(m_firstIndex + i) + t) * ROAD_STEP;
            result.d = dx * cosf(heading) - dz * sinf(heading);     // dot with the right-hand normal
        }
    }

    return result;
}

void Road::AppendPoint()
{
    RoadPoint last, next;
    float headingMid, magnitude, difference, maxChange;

    last = m_points.back();

    // Walk one step along the current curve (midpoint heading keeps the arc accurate).
    headingMid = last.heading + 0.5f * last.curvature * ROAD_STEP;

    next.x = last.x + sinf(headingMid) * ROAD_STEP;
    next.z = last.z + cosf(headingMid) * ROAD_STEP;
    next.heading = last.heading + last.curvature * ROAD_STEP;

    // Time for a new bend?
    m_segmentRemaining -= ROAD_STEP;
    if(m_segmentRemaining <= 0.0f)
    {
        m_segmentRemaining = SEGMENT_MIN + NextRandom() * (SEGMENT_MAX - SEGMENT_MIN);
        magnitude = MIN_BEND_FRACTION + NextRandom() * (1.0f - MIN_BEND_FRACTION);

        // Mostly alternate left / right, sometimes keep bending the same way.
        if(NextRandom() < 0.8f)
        {
            m_sign = -m_sign;
        }

        m_targetCurvature = m_sign * magnitude * MAX_CURVATURE;
    }

    // Ease the curvature toward the target so the road never kinks.
    difference = m_targetCurvature - last.curvature;
    maxChange = CURVATURE_RATE * ROAD_STEP;

    if(difference > maxChange)  { difference = maxChange; }
    if(difference < -maxChange) { difference = -maxChange; }

    next.curvature = last.curvature + difference;

    m_points.push_back(next);
}

float Road::NextRandom()
{
    // xorshift32: tiny deterministic generator, the same seed always gives the same road.
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;

    return (float)(m_rng & 0xFFFFFF) / 16777216.0f;
}

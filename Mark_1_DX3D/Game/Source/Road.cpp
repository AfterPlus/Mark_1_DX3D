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

// Elevation. The road is split into zones of ZONE_LENGTH +- ZONE_JITTER metres; each zone rolls its own landscape.
static const float ZONE_LENGTH = 300.0f;
static const float ZONE_JITTER = 0.2f;          // boundaries move up to +-20% of a zone, so zones are 180..420 m long (always longer than ROAD_BLEND)
static const float ROAD_BLEND = 170.0f;         // metres over which the road's hilliness changes between zones (keeps grades gentle)
static const float SIDE_BLEND = 50.0f;          // metres over which the land beside the road changes between zones
static const float CALM_CHANCE = 0.2f;          // chance a zone is completely flat: level road, plains on both sides

// Road profile: three long sine waves along s, scaled per zone by 0 (flat) .. 1 (fully rolling, ~8% grades).
//                                               wavelength (m)     amplitude (m)
static const float ROAD_HILL_LENGTH[3] =        { 620.0f, 310.0f, 170.0f };
static const float ROAD_HILL_HEIGHT[3] =        { 6.0f,   2.5f,   0.8f };
static const float HILLS_FLAT_START = 200.0f;   // the road starts level at s = 0 and grows into its hills over this distance

// Land beside the road stays level with it for SIDE_SHOULDER metres past the edge, then takes the zone's shape.
static const float SIDE_SHOULDER = 2.0f;

enum SideType
{
    SIDE_PLAIN,         // flat, a hint of undulation
    SIDE_HILLS,         // rolling hills, up and down
    SIDE_BANK,          // the land rises into a bank / hillside
    SIDE_DROP,          // a sudden deep drop just past the shoulder
    SIDE_VALLEY,        // the land falls gently away
};

// How often each side type is picked (sums to 1), in SideType order.
static const float SIDE_CHANCE[5] = { 0.30f, 0.25f, 0.15f, 0.15f, 0.15f };

static float SmoothStep(float edge0, float edge1, float x)
{
    float t = (x - edge0) / (edge1 - edge0);

    if(t < 0.0f) { t = 0.0f; }
    if(t > 1.0f) { t = 1.0f; }

    return t * t * (3.0f - 2.0f * t);
}

// Stateless random number 0..1 for (seed, zone, salt): the same inputs always give the same value.
static float Hash01(unsigned int seed, int zone, unsigned int salt)
{
    unsigned int x = seed * 0x9E3779B1u ^ (unsigned int)zone * 0x85EBCA77u ^ salt * 0xC2B2AE3Du;

    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;

    return (float)(x & 0xFFFFFF) / 16777216.0f;
}

// Where zone `zone` begins.
static float ZoneStart(unsigned int seed, int zone)
{
    return ((float)zone + (Hash01(seed, zone, 1) - 0.5f) * 2.0f * ZONE_JITTER) * ZONE_LENGTH;
}

// The two zones that make up the landscape at s and how much of the second one shows (0..1). Within `blend` metres of a
// boundary the two zones on either side of it are mixed; elsewhere both are the zone s is in.
static void ZoneMix(unsigned int seed, float s, float blend, int& zoneA, int& zoneB, float& weight)
{
    int zone = (int)floorf(s / ZONE_LENGTH);
    float start, end;

    if(s < ZoneStart(seed, zone))              { zone--; }
    else if(s >= ZoneStart(seed, zone + 1))    { zone++; }

    start = ZoneStart(seed, zone);
    end = ZoneStart(seed, zone + 1);

    zoneA = zone;
    zoneB = zone;
    weight = 0.0f;

    if(s < start + 0.5f * blend)
    {
        zoneA = zone - 1;
        weight = SmoothStep(start - 0.5f * blend, start + 0.5f * blend, s);
    }
    else if(s > end - 0.5f * blend)
    {
        zoneB = zone + 1;
        weight = SmoothStep(end - 0.5f * blend, end + 0.5f * blend, s);
    }
}

// A whole zone is flat (the start zone always is).
static bool ZoneIsCalm(unsigned int seed, int zone)
{
    return zone <= 0 || Hash01(seed, zone, 2) < CALM_CHANCE;
}

// How hilly the road itself is in a zone: 0 = flat .. 1 = fully rolling.
static float ZoneRoadHills(unsigned int seed, int zone)
{
    float r;

    if(ZoneIsCalm(seed, zone))
    {
        return 0.0f;
    }

    r = Hash01(seed, zone, 3);

    if(r < 0.3f) { return 0.0f; }                                       // flat road (the sides may still have hills)
    if(r < 0.7f) { return 0.3f + 0.4f * Hash01(seed, zone, 4); }       // gently rolling

    return 0.75f + 0.25f * Hash01(seed, zone, 5);                       // properly hilly
}

// Height of the land on one side (side = -1 left, +1 right) relative to the road, for one zone.
// u = metres past the shoulder (> 0).
static float ZoneSideHeight(unsigned int seed, int zone, int side, float s, float u)
{
    unsigned int salt = (side < 0) ? 100 : 200;
    float pick, a, b, c, phase, length, height, width;
    int type;

    // Pick the type.
    type = SIDE_PLAIN;

    if(!ZoneIsCalm(seed, zone))
    {
        pick = Hash01(seed, zone, salt);

        for(type=0; type<4; type++)
        {
            if(pick < SIDE_CHANCE[type]) { break; }
            pick -= SIDE_CHANCE[type];
        }
    }

    // Random size / shape of it.
    a = Hash01(seed, zone, salt + 1);
    b = Hash01(seed, zone, salt + 2);
    c = Hash01(seed, zone, salt + 3);
    phase = Hash01(seed, zone, salt + 4) * 6.2831853f;

    switch(type)
    {
        case SIDE_HILLS:
        {
            height = 3.0f + 7.0f * a;
            length = 50.0f + 110.0f * b;

            return height * SmoothStep(0.0f, 12.0f + 20.0f * c, u) *
                   (0.65f * sinf(s * (6.2831853f / length) + u * 0.06f + phase) +
                    0.35f * sinf(s * (6.2831853f / (0.43f * length)) - u * 0.11f + 2.0f * phase));
        }

        case SIDE_BANK:
        {
            height = 5.0f + 12.0f * a;
            width = 8.0f + 25.0f * b;

            return SmoothStep(0.0f, width, u) * (height + 1.5f * sinf(s * (6.2831853f / 70.0f) + u * 0.08f + phase));
        }

        case SIDE_DROP:
        {
            height = 4.0f + 6.0f * a;
            width = 5.0f + 6.0f * b;

            return -height * SmoothStep(0.0f, width, u) + 1.0f * sinf(s * 0.07f + u * 0.05f + phase) * SmoothStep(width, width + 10.0f, u);
        }

        case SIDE_VALLEY:
        {
            height = 3.0f + 5.0f * a;
            width = 20.0f + 25.0f * b;

            return SmoothStep(0.0f, width, u) * (-height + 0.8f * sinf(s * 0.05f + u * 0.07f + phase));
        }

        default:
        {
            return 0.5f * a * sinf(s * 0.05f + u * 0.04f + phase) * SmoothStep(0.0f, 15.0f, u);
        }
    }
}

Road::Road()
{
    m_firstIndex = 0;
    m_targetCurvature = 0.0f;
    m_segmentRemaining = 0.0f;
    m_sign = 1.0f;
    m_rng = 1;

    m_terrainSeed = 1;

    for(int i=0; i<3; i++)
    {
        m_hillPhase[i] = 0.0f;
    }
}

void Road::Reset(unsigned int seed)
{
    RoadPoint first;

    // The hill phases come from their own generator so the bends of the road are the same as before.
    m_rng = (seed == 0) ? 1 : seed;
    m_rng ^= 0x9E3779B9u;
    for(int i=0; i<3; i++)
    {
        m_hillPhase[i] = NextRandom() * 6.2831853f;
    }
    m_terrainSeed = m_rng;

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

float Road::GetHeight(float s) const
{
    float height = 0.0f, hills, weight;
    int i, zoneA, zoneB;

    for(i=0; i<3; i++)
    {
        height += ROAD_HILL_HEIGHT[i] * sinf(s * (6.2831853f / ROAD_HILL_LENGTH[i]) + m_hillPhase[i]);
    }

    // How hilly this stretch is, blended slowly between zones.
    ZoneMix(m_terrainSeed, s, ROAD_BLEND, zoneA, zoneB, weight);
    hills = ZoneRoadHills(m_terrainSeed, zoneA) + (ZoneRoadHills(m_terrainSeed, zoneB) - ZoneRoadHills(m_terrainSeed, zoneA)) * weight;

    // Level at the start line, then ease into the hills (and level behind the start too).
    return height * hills * SmoothStep(0.0f, HILLS_FLAT_START, s);
}

float Road::GetGrade(float s) const
{
    return (GetHeight(s + 0.5f) - GetHeight(s - 0.5f));
}

float Road::GetSurfaceHeight(float s, float d) const
{
    float u, weight, sideA, sideB;
    int side, zoneA, zoneB;

    u = fabsf(d) - ROAD_HALF_WIDTH - SIDE_SHOULDER;
    if(u <= 0.0f)
    {
        return GetHeight(s);
    }

    side = (d < 0.0f) ? -1 : 1;

    ZoneMix(m_terrainSeed, s, SIDE_BLEND, zoneA, zoneB, weight);
    sideA = ZoneSideHeight(m_terrainSeed, zoneA, side, s, u);
    sideB = (zoneB == zoneA) ? sideA : ZoneSideHeight(m_terrainSeed, zoneB, side, s, u);

    return GetHeight(s) + sideA + (sideB - sideA) * weight;
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

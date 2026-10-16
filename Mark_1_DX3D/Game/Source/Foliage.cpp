#include <stdio.h>
#include "Foliage.h"

static const char* GRASS_TEXTURE_FILE = "Resources/Grass/Texture/Grass_BaseColor_Alpha.tga";

// Grass is placed in 1 m slices along the road; each slice and side gets its own random numbers.
static const float CELL_LENGTH = 1.0f;
static const float EDGE_GAP = 0.6f;             // bare strip between the road edge and the first grass
static const float FIELD_WIDTH = 32.0f;         // how far out from the road the grass goes
static const float FIELD_FALLOFF = 1.7f;        // > 1 = denser near the road, thinner further out
static const float ROOT_SINK = 0.03f;           // pushes roots a little into the ground so none float on slopes

// Wind.
static const float WIND_ANGLE = 0.6f;           // radians from +X; the direction drifts slowly around it
static const float WIND_ANGLE_DRIFT = 0.35f;
static const float WIND_STRENGTH = 0.28f;
static const float WIND_SPEED = 1.8f;

// Far grass shrinks away between these distances from the camera (the placed stretch reaches a bit further).
static const float FADE_START = 110.0f;
static const float FADE_END = 145.0f;

// Small integer hash -> well-mixed bits. Same input, same output, on every run.
static unsigned int Hash(unsigned int x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;

    return x;
}

// A random sequence seeded by one slice of road.
struct CellRandom
{
    unsigned int state;

    float Next()            // 0..1
    {
        state = Hash(state + 0x9e3779b9U);
        return (float)(state & 0xffffff) / (float)0x1000000;
    }

    float Range(float low, float high)
    {
        return low + (high - low) * Next();
    }
};

// Smooth 0..1 pattern over the land, used for patches (thick / thin grass, green / dry).
static float Patch(float s, float d, float scale, float offset)
{
    float value = sinf(s * 0.09f * scale + offset) * cosf(s * 0.031f * scale - d * 0.13f * scale + offset * 1.7f) +
                  0.5f * sinf(d * 0.07f * scale + s * 0.05f * scale + offset * 0.3f);

    return value * (1.0f / 3.0f) + 0.5f;
}

static float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

Foliage::Foliage()
{
    // name, tufts per metre per side, height range (m), width scale range.
    FoliageType types[TYPE_COUNT] =
    {
        { "grass_clump_01", 0.9f, 0.35f, 0.85f, 0.8f, 1.3f, nullptr },
        { "grass_clump_02", 0.8f, 0.30f, 0.75f, 0.8f, 1.3f, nullptr },
        { "grass_clump_03", 0.8f, 0.40f, 0.95f, 0.8f, 1.2f, nullptr },
        { "grass_blade_01", 2.0f, 0.30f, 0.90f, 0.8f, 1.6f, nullptr },
        { "grass_blade_02", 2.0f, 0.25f, 0.75f, 0.8f, 1.6f, nullptr },
        { "grass_blade_03", 2.0f, 0.35f, 1.10f, 0.8f, 1.6f, nullptr },
    };
    int i;

    for(i=0; i<TYPE_COUNT; i++)
    {
        m_types[i] = types[i];
    }

    m_textureLoaded = false;
    m_ready = false;
    m_time = 0.0f;
}

bool Foliage::Initialize(RendererClass* renderer)
{
    ID3D11Device* device = renderer->GetDirect3D()->GetDevice();
    ID3D11DeviceContext* context = renderer->GetDirect3D()->GetDeviceContext();
    char filename[256];
    int i, loaded;

    strcpy_s(filename, GRASS_TEXTURE_FILE);

    m_textureLoaded = m_texture.Initialize(device, context, filename);
    if(!m_textureLoaded)
    {
        return false;
    }

    // Every model shares the one grass texture.
    loaded = 0;

    for(i=0; i<TYPE_COUNT; i++)
    {
        sprintf_s(filename, "Resources/Models/%s.txt", m_types[i].name);

        m_types[i].model = new ModelClass;

        if(!m_types[i].model->InitializeShared(device, filename, m_texture.GetTexture()))
        {
            // Not fatal: the other kinds of grass still grow.
            delete m_types[i].model;
            m_types[i].model = nullptr;
            continue;
        }

        loaded++;
    }

    // One batch per model; Rebuild fills them.
    m_scene.batches.resize(TYPE_COUNT);

    for(i=0; i<TYPE_COUNT; i++)
    {
        m_scene.batches[i].model = m_types[i].model;
    }

    m_ready = loaded > 0;

    Update(0.0f);

    return m_ready;
}

void Foliage::Shutdown()
{
    int i;

    m_scene.batches.clear();

    for(i=0; i<TYPE_COUNT; i++)
    {
        if(m_types[i].model)
        {
            m_types[i].model->Shutdown();
            delete m_types[i].model;
            m_types[i].model = nullptr;
        }
    }

    if(m_textureLoaded)
    {
        m_texture.Shutdown();
        m_textureLoaded = false;
    }

    m_ready = false;
}

void Foliage::Rebuild(const Road& road, float startS, float endS)
{
    size_t i;
    int cell, firstCell, lastCell;

    if(!m_ready)
    {
        return;
    }

    for(i=0; i<m_scene.batches.size(); i++)
    {
        m_scene.batches[i].instances.clear();
    }

    // Only where the road exists.
    if(startS < road.GetMinS())
    {
        startS = road.GetMinS();
    }

    if(endS > road.GetMaxS())
    {
        endS = road.GetMaxS();
    }

    firstCell = (int)ceilf(startS / CELL_LENGTH);
    lastCell = (int)floorf(endS / CELL_LENGTH) - 1;

    for(cell=firstCell; cell<=lastCell; cell++)
    {
        PlaceSide(road, cell, -1);
        PlaceSide(road, cell, 1);
    }
}

// The grass of one 1 m slice of road on one side (-1 = left, +1 = right).
void Foliage::PlaceSide(const Road& road, int cell, int side)
{
    FoliageInstance instance;
    CellRandom random;
    RoadPoint point;
    float s, d, patch, dryness, count, shade;
    int type, n, copies;

    random.state = Hash((unsigned int)cell * 2U + (side > 0 ? 1U : 0U));

    for(type=0; type<TYPE_COUNT; type++)
    {
        if(!m_types[type].model)
        {
            continue;
        }

        // Whole tufts plus a chance of one more for the fraction.
        count = m_types[type].density;
        copies = (int)count;
        if(random.Next() < count - (float)copies)
        {
            copies++;
        }

        for(n=0; n<copies; n++)
        {
            s = ((float)cell + random.Next()) * CELL_LENGTH;
            d = ROAD_HALF_WIDTH + EDGE_GAP + FIELD_WIDTH * powf(random.Next(), FIELD_FALLOFF);

            // Patches of thick and thin grass: thin patches drop some tufts and keep the rest shorter.
            patch = Patch(s, d * (float)side, 1.0f, 0.0f);
            if(random.Next() > 0.3f + 0.7f * patch)
            {
                continue;
            }

            point = road.GetPoint(s);
            d *= (float)side;

            instance.position = XMFLOAT3(point.x + cosf(point.heading) * d,
                                         road.GetSurfaceHeight(s, d) - ROOT_SINK,
                                         point.z - sinf(point.heading) * d);

            instance.yaw = random.Range(0.0f, XM_2PI);
            instance.height = random.Range(m_types[type].minHeight, m_types[type].maxHeight) * (0.75f + 0.4f * patch);
            instance.width = random.Range(m_types[type].minWidth, m_types[type].maxWidth);
            instance.phase = random.Range(0.0f, XM_2PI);
            instance.stiffness = random.Range(0.7f, 1.0f);

            // Colour: a little random variation per tuft, and larger dry (yellow) areas.
            dryness = Patch(s, d, 0.6f, 4.0f);
            dryness = dryness * dryness * 0.7f;
            shade = random.Range(0.85f, 1.1f);

            instance.tint = XMFLOAT4(Lerp(random.Range(0.9f, 1.05f), 1.3f, dryness) * shade,
                                     Lerp(random.Range(0.95f, 1.1f), 1.1f, dryness) * shade,
                                     Lerp(random.Range(0.85f, 1.0f), 0.55f, dryness) * shade,
                                     1.0f);

            m_scene.batches[type].instances.push_back(instance);
        }
    }
}

void Foliage::Update(float dt)
{
    float angle;

    m_time += dt;

    angle = WIND_ANGLE + WIND_ANGLE_DRIFT * sinf(m_time * 0.04f);

    m_scene.wind.direction = XMFLOAT2(cosf(angle), sinf(angle));
    m_scene.wind.strength = WIND_STRENGTH;
    m_scene.wind.speed = WIND_SPEED;
    m_scene.wind.time = m_time;
    m_scene.wind.fadeStart = FADE_START;
    m_scene.wind.fadeEnd = FADE_END;
}

const FoliageScene* Foliage::GetScene() const
{
    return m_ready ? &m_scene : nullptr;
}

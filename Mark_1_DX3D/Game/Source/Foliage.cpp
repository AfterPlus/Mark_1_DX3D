#include <stdio.h>
#include <algorithm>
#include "Foliage.h"

static const char* GRASS_TEXTURE_FILE = "Resources/Grass/Texture/Grass_BaseColor_Alpha.tga";

// Grass is placed in 1 m cells along the road; each cell and side gets its own random numbers.
static const float CELL_LENGTH = 1.0f;
static const int   CHUNK_CELLS = 16;            // a chunk is this many cells long
static const float CHUNK_LENGTH = CELL_LENGTH * CHUNK_CELLS;
static const float EDGE_GAP = 1.5f;             // bare strip between the road edge and the first grass
static const float FIELD_WIDTH = 50.0f;         // how far out from the road the grass goes (the ground ends ~53 m past the road edge)
static const float FIELD_FALLOFF = 1.3f;        // > 1 = denser near the road, thinner further out
static const float ROOT_SINK = 0.03f;           // pushes roots a little into the ground so none float on slopes

// Chunk bands, in metres past the road edge: [0, 8), [8, 20), [20, ...).
static const float BAND_EDGES[2] = { 8.0f, 20.0f };

// Wind.
static const float WIND_ANGLE = 0.6f;           // radians from +X; the direction drifts slowly around it
static const float WIND_ANGLE_DRIFT = 0.35f;
static const float WIND_STRENGTH = 0.28f;
static const float WIND_SPEED = 1.8f;

// Far grass shrinks away between these distances from the camera (the placed stretch reaches a bit further).
static const float FADE_START = 115.0f;         // must stay below FADE_END
static const float FADE_END = 150.0f;

// Level of detail: past a model's lodDistance only (lodDistance / distance)^2 of its instances are drawn, never less
// than LOD_MIN_KEEP, and the ones left are made up to LOD_MAX_WIDEN times wider to fill the gaps.
static const float LOD_MIN_KEEP = 0.02f;
static const float LOD_MAX_WIDEN = 2.5f;

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

// A random sequence seeded by one cell of road.
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

static bool ByLodRank(const FoliageInstance& a, const FoliageInstance& b)
{
    return a.lodRank < b.lodRank;
}

Foliage::Foliage()
{
    // name, tufts per metre per side, height range (m), width scale range, LOD distance (m).
    // Clumps are ~10x the triangles of a blade, so they thin out much closer to the camera.
    FoliageType types[TYPE_COUNT] =
    {
        { "grass_clump_01", 100.0f, 0.35f, 0.85f, 0.8f, 1.3f, 12.0f, nullptr },
        { "grass_clump_02", 100.0f, 0.30f, 0.75f, 0.8f, 1.3f, 12.0f, nullptr },
        { "grass_clump_03", 100.0f, 0.40f, 0.95f, 0.8f, 1.2f, 12.0f, nullptr },
        { "grass_blade_01", 120.0f, 0.30f, 0.90f, 0.8f, 1.6f, 20.0f, nullptr },
        { "grass_blade_02",  90.0f, 0.25f, 0.75f, 0.8f, 1.6f, 20.0f, nullptr },
        { "grass_blade_03",  60.0f, 0.35f, 1.10f, 0.8f, 1.6f, 20.0f, nullptr },
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

    m_scene.models.resize(TYPE_COUNT);

    for(i=0; i<TYPE_COUNT; i++)
    {
        sprintf_s(filename, "Resources/Models/%s.txt", m_types[i].name);

        m_types[i].model = new ModelClass;

        if(!m_types[i].model->InitializeShared(device, filename, m_texture.GetTexture()))
        {
            // Not fatal: the other kinds of grass still grow.
            delete m_types[i].model;
            m_types[i].model = nullptr;
        }
        else
        {
            loaded++;
        }

        m_scene.models[i].model = m_types[i].model;
        m_scene.models[i].lodDistance = m_types[i].lodDistance;
    }

    m_ready = loaded > 0;

    Update(0.0f);

    return m_ready;
}

void Foliage::Shutdown()
{
    int i;

    m_chunks.clear();
    m_scene.models.clear();
    m_scene.instances.clear();
    m_scene.chunks.clear();

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
    std::map<int, Chunk>::iterator it;
    int firstSlice, lastSlice, slice, side, key;
    bool changed;

    if(!m_ready)
    {
        return;
    }

    // Only whole chunks of road that exists.
    if(startS < road.GetMinS())
    {
        startS = road.GetMinS();
    }

    if(endS > road.GetMaxS())
    {
        endS = road.GetMaxS();
    }

    firstSlice = (int)ceilf(startS / CHUNK_LENGTH);
    lastSlice = (int)floorf(endS / CHUNK_LENGTH) - 1;

    changed = false;

    // Drop what is out of range ...
    for(it = m_chunks.begin(); it != m_chunks.end(); )
    {
        slice = (it->first >= 0 ? it->first : it->first - 1) / 2;

        if(slice < firstSlice || slice > lastSlice)
        {
            it = m_chunks.erase(it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }

    // ... and build what is new.
    for(slice=firstSlice; slice<=lastSlice; slice++)
    {
        for(side=-1; side<=1; side+=2)
        {
            key = slice * 2 + (side > 0 ? 1 : 0);

            if(m_chunks.find(key) == m_chunks.end())
            {
                BuildChunk(road, slice, side, m_chunks[key]);
                changed = true;
            }
        }
    }

    if(changed)
    {
        Flatten();
    }
}

void Foliage::BuildChunk(const Road& road, int slice, int side, Chunk& chunk)
{
    XMFLOAT3 low, high;
    float top;
    int cell, band, type;
    size_t i;

    for(cell=slice * CHUNK_CELLS; cell<(slice + 1) * CHUNK_CELLS; cell++)
    {
        PlaceCell(road, cell, side, chunk);
    }

    for(band=0; band<BAND_COUNT; band++)
    {
        low = XMFLOAT3(1e9f, 1e9f, 1e9f);
        high = XMFLOAT3(-1e9f, -1e9f, -1e9f);
        top = 0.0f;

        for(type=0; type<TYPE_COUNT; type++)
        {
            std::vector<FoliageInstance>& list = chunk.instances[band][type];

            // Lowest rank first: the renderer draws the first N of a chunk when it thins it out.
            std::sort(list.begin(), list.end(), ByLodRank);

            for(i=0; i<list.size(); i++)
            {
                low.x = min(low.x, list[i].position.x);
                low.y = min(low.y, list[i].position.y);
                low.z = min(low.z, list[i].position.z);
                high.x = max(high.x, list[i].position.x);
                high.y = max(high.y, list[i].position.y);
                high.z = max(high.z, list[i].position.z);
                top = max(top, list[i].height);
            }
        }

        if(low.x > high.x)
        {
            chunk.center[band] = XMFLOAT3(0.0f, 0.0f, 0.0f);
            chunk.radius[band] = 0.0f;
            continue;
        }

        // Roots' box, raised by the tallest tuft; the margin covers the widest (LOD-widened) tuft and the wind lean.
        high.y += top;

        chunk.center[band] = XMFLOAT3((low.x + high.x) * 0.5f, (low.y + high.y) * 0.5f, (low.z + high.z) * 0.5f);
        chunk.radius[band] = 0.5f * sqrtf((high.x - low.x) * (high.x - low.x) + (high.y - low.y) * (high.y - low.y) +
                                          (high.z - low.z) * (high.z - low.z)) + top * LOD_MAX_WIDEN;
    }
}

// The grass of one 1 m cell of road on one side (-1 = left, +1 = right).
void Foliage::PlaceCell(const Road& road, int cell, int side, Chunk& chunk)
{
    FoliageInstance instance;
    CellRandom random;
    RoadPoint point;
    float s, edge, d, patch, dryness, count, shade;
    int type, n, copies, band;

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
            edge = EDGE_GAP + FIELD_WIDTH * powf(random.Next(), FIELD_FALLOFF);     // metres past the road edge
            d = ROAD_HALF_WIDTH + edge;

            // Patches of thick and thin grass: thin patches drop some tufts and keep the rest shorter.
            patch = Patch(s, d * (float)side, 1.0f, 0.0f);
            if(random.Next() > 0.55f + 0.45f * patch)
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

            instance.tint = XMFLOAT3(Lerp(random.Range(0.9f, 1.05f), 1.3f, dryness) * shade,
                                     Lerp(random.Range(0.95f, 1.1f), 1.1f, dryness) * shade,
                                     Lerp(random.Range(0.85f, 1.0f), 0.55f, dryness) * shade);

            instance.lodRank = random.Next();

            band = edge < BAND_EDGES[0] ? 0 : (edge < BAND_EDGES[1] ? 1 : 2);

            chunk.instances[band][type].push_back(instance);
        }
    }
}

// Lays every chunk's instances out back to back for the renderer (one GPU upload), one FoliageChunk per band and model.
void Foliage::Flatten()
{
    std::map<int, Chunk>::iterator it;
    FoliageChunk entry;
    size_t total;
    int band, type;

    total = 0;
    for(it = m_chunks.begin(); it != m_chunks.end(); ++it)
    {
        for(band=0; band<BAND_COUNT; band++)
        {
            for(type=0; type<TYPE_COUNT; type++)
            {
                total += it->second.instances[band][type].size();
            }
        }
    }

    m_scene.instances.clear();
    m_scene.instances.reserve(total);
    m_scene.chunks.clear();

    for(it = m_chunks.begin(); it != m_chunks.end(); ++it)
    {
        for(band=0; band<BAND_COUNT; band++)
        {
            for(type=0; type<TYPE_COUNT; type++)
            {
                const std::vector<FoliageInstance>& list = it->second.instances[band][type];

                if(list.empty())
                {
                    continue;
                }

                entry.model = type;
                entry.first = (int)m_scene.instances.size();
                entry.count = (int)list.size();
                entry.center = it->second.center[band];
                entry.radius = it->second.radius[band];

                m_scene.chunks.push_back(entry);
                m_scene.instances.insert(m_scene.instances.end(), list.begin(), list.end());
            }
        }
    }

    m_scene.version++;
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
    m_scene.wind.lodMinKeep = LOD_MIN_KEEP;
    m_scene.wind.lodMaxWiden = LOD_MAX_WIDEN;
}

const FoliageScene* Foliage::GetScene() const
{
    return m_ready ? &m_scene : nullptr;
}

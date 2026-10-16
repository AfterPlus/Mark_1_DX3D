#pragma once

#include <windows.h>
#include <map>
#include <vector>
#include "RendererClass.h"
#include "TextureClass.h"
#include "Road.h"

// Grass on both sides of the endless road. Every grass model in Resources/Grass (converted by
// Tools/blender/dh_export_grass.py) is scattered beside the road with its own size, rotation, tint and wind phase.
// Placement is a pure function of the road distance, so the same tufts are always in the same places.
//
// The grass is kept in chunks (CHUNK_LENGTH metres of road, one side, one band of distance from the road). A chunk is
// built once when it comes into range and dropped when it leaves, so driving only ever builds the few new chunks.
// The renderer culls and thins whole chunks (see RendererClass::RenderFoliage).
class Foliage
{
public:
    Foliage();

    // Loads the models and the shared grass texture. Returns false (and draws nothing) when they are missing;
    // the game runs fine without grass.
    bool Initialize(RendererClass* renderer);
    void Shutdown();

    // Makes sure the grass exists between road distances startS and endS (builds new chunks, drops old ones).
    void Rebuild(const Road& road, float startS, float endS);

    // Advances the wind.
    void Update(float dt);

    // nullptr when there is nothing to draw.
    const FoliageScene* GetScene() const;

private:
    enum { TYPE_COUNT = 6, BAND_COUNT = 3 };

    struct FoliageType
    {
        const char* name;       // Resources/Models/<name>.txt
        float density;          // tufts per metre of road, per side
        float minHeight, maxHeight;     // metres
        float minWidth, maxWidth;       // sideways scale
        float lodDistance;      // metres from the camera where it starts thinning out (heavy models thin out sooner)
        ModelClass* model;
    };

    // One stretch of road on one side. Its grass is split into bands by distance from the road, each band with its own
    // bounding sphere, so the near band and the far band are culled and thinned separately.
    struct Chunk
    {
        std::vector<FoliageInstance> instances[BAND_COUNT][TYPE_COUNT];   // each sorted by lodRank
        XMFLOAT3 center[BAND_COUNT];
        float radius[BAND_COUNT];
    };

    void BuildChunk(const Road& road, int slice, int side, Chunk& chunk);
    void PlaceCell(const Road& road, int cell, int side, Chunk& chunk);
    void Flatten();

private:
    FoliageType m_types[TYPE_COUNT];
    TextureClass m_texture;
    bool m_textureLoaded;
    bool m_ready;
    std::map<int, Chunk> m_chunks;      // key = slice * 2 + (side > 0)
    FoliageScene m_scene;
    float m_time;
};

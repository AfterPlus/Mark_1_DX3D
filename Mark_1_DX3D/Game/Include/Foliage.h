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
// The grass is kept in chunks (CHUNK_LENGTH metres of road on one side, split into bands of distance from the road).
// A chunk is built once when it comes into range and dropped when it leaves, so driving only ever builds the few new
// chunks. Each chunk owns a fixed-size slot of the instance buffer, so a new chunk is written into a free slot and only
// that slot is uploaded to the GPU. The renderer culls and thins each band (see RendererClass::RenderFoliage).
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

    // nullptr when there is nothing to draw. Not const: the renderer clears the scene's list of changed instances.
    FoliageScene* GetScene();

private:
    enum { TYPE_COUNT = 6, BAND_COUNT = 3 };

    struct FoliageType
    {
        const char* name;       // Resources/Models/<name>.txt
        float density;          // tufts per metre of road, per side
        float minHeight, maxHeight;     // metres
        float minWidth, maxWidth;       // sideways scale
        float lodDistance;      // metres from the camera where it starts thinning out (heavy models thin out sooner) ...
        float lodMinKeep;       // ... down to this share far away
        ModelClass* model;
    };

    // One stretch of road on one side. Its grass is split into bands by distance from the road, each band with its own
    // bounding sphere, so the near band and the far band are culled and thinned separately.
    struct Chunk
    {
        int slot;                                   // its part of m_scene.instances: [slot * m_slotSize, ...)
        int first[BAND_COUNT][TYPE_COUNT];          // where each band / model starts in m_scene.instances ...
        int count[BAND_COUNT][TYPE_COUNT];          // ... and how many (sorted by lodRank)
        XMFLOAT3 center[BAND_COUNT];
        float radius[BAND_COUNT];
    };

    void BuildChunk(const Road& road, int slice, int side, Chunk& chunk);
    void PlaceCell(const Road& road, int cell, int side);
    int AllocateSlot();
    void UpdateChunkList();

private:
    FoliageType m_types[TYPE_COUNT];
    TextureClass m_texture;
    bool m_textureLoaded;
    bool m_ready;
    std::map<int, Chunk> m_chunks;      // key = slice * 2 + (side > 0)
    int m_slotSize;                     // most instances one chunk can have
    std::vector<int> m_freeSlots;
    std::vector<FoliageInstance> m_build[BAND_COUNT][TYPE_COUNT];   // scratch space while a chunk is built
    FoliageScene m_scene;
    float m_time;
};

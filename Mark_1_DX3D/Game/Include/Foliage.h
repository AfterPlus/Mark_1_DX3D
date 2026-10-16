#pragma once

#include <windows.h>
#include "RendererClass.h"
#include "TextureClass.h"
#include "Road.h"

// Grass on both sides of the endless road. Every grass model in Resources/Grass (converted by
// Tools/blender/dh_export_grass.py) is scattered beside the road with its own size, rotation, tint and wind phase.
// Placement is a pure function of the road distance, so the same tufts are always in the same places while the
// placed stretch slides along with the car.
class Foliage
{
public:
    Foliage();

    // Loads the models and the shared grass texture. Returns false (and draws nothing) when they are missing;
    // the game runs fine without grass.
    bool Initialize(RendererClass* renderer);
    void Shutdown();

    // Re-places the grass between road distances startS and endS.
    void Rebuild(const Road& road, float startS, float endS);

    // Advances the wind.
    void Update(float dt);

    // nullptr when there is nothing to draw.
    const FoliageScene* GetScene() const;

private:
    enum { TYPE_COUNT = 6 };

    struct FoliageType
    {
        const char* name;       // Resources/Models/<name>.txt
        float density;          // tufts per metre of road, per side
        float minHeight, maxHeight;     // metres
        float minWidth, maxWidth;       // sideways scale
        ModelClass* model;
    };

    void PlaceSide(const Road& road, int cell, int side);

private:
    FoliageType m_types[TYPE_COUNT];
    TextureClass m_texture;
    bool m_textureLoaded;
    bool m_ready;
    FoliageScene m_scene;
    float m_time;
};

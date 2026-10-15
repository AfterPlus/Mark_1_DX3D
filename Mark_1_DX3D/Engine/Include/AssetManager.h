#pragma once

#include <map>
#include <set>
#include <string>
#include <directxmath.h>
#include "ModelClass.h"
#include "TextureClass.h"

using namespace DirectX;

// A model from Resources/Models plus what the exporter wrote into its .meta file.
struct ModelAsset
{
    ModelClass* model;
    XMFLOAT3 boundsMin, boundsMax;                  // metres, model space (origin on the ground, +Z = nose / front)
    int triangles;
    std::map<std::string, XMFLOAT3> anchors;        // named points: wheel_fl, ...
};

// Loads every model once and shares it. All models are painted from ONE palette texture
// (Resources/Textures/palette.tga), so the manager keeps that single texture and hands it to every model.
class AssetManager
{
public:
    AssetManager();
    ~AssetManager();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    void Shutdown();

    // name = file name without extension, e.g. "car_test" -> Resources/Models/car_test.txt (+ .meta).
    // Loaded on first use. Returns nullptr when the model is missing or broken (and does not retry).
    const ModelAsset* GetModel(const std::string& name);

private:
    bool LoadMeta(const std::string& path, ModelAsset& asset);

private:
    ID3D11Device* m_device;
    TextureClass m_palette;
    bool m_paletteLoaded;
    std::map<std::string, ModelAsset*> m_models;
    std::set<std::string> m_failed;
};

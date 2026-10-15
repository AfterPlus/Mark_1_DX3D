#include "AssetManager.h"
#include <fstream>

static const char* MODEL_DIRECTORY = "Resources/Models/";
static const char* PALETTE_FILE = "Resources/Textures/palette.tga";

AssetManager::AssetManager()
{
    m_device = nullptr;
    m_paletteLoaded = false;
}

AssetManager::~AssetManager()
{
}

bool AssetManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    char filename[128];

    m_device = device;

    strcpy_s(filename, PALETTE_FILE);

    m_paletteLoaded = m_palette.Initialize(device, deviceContext, filename);

    return m_paletteLoaded;
}

void AssetManager::Shutdown()
{
    std::map<std::string, ModelAsset*>::iterator it;

    for(it = m_models.begin(); it != m_models.end(); ++it)
    {
        it->second->model->Shutdown();
        delete it->second->model;
        delete it->second;
    }

    m_models.clear();
    m_failed.clear();

    if(m_paletteLoaded)
    {
        m_palette.Shutdown();
        m_paletteLoaded = false;
    }
}

const ModelAsset* AssetManager::GetModel(const std::string& name)
{
    std::map<std::string, ModelAsset*>::iterator found = m_models.find(name);
    std::string path;
    char filename[256];
    ModelAsset* asset;

    if(found != m_models.end())
    {
        return found->second;
    }

    if(!m_paletteLoaded || m_failed.count(name) > 0)
    {
        return nullptr;
    }

    path = std::string(MODEL_DIRECTORY) + name;
    strcpy_s(filename, (path + ".txt").c_str());

    asset = new ModelAsset;
    asset->model = new ModelClass;
    asset->boundsMin = XMFLOAT3(0.0f, 0.0f, 0.0f);
    asset->boundsMax = XMFLOAT3(0.0f, 0.0f, 0.0f);
    asset->triangles = 0;

    if(!asset->model->InitializeShared(m_device, filename, m_palette.GetTexture()))
    {
        // Not fatal: the caller can fall back to a placeholder.
        delete asset->model;
        delete asset;
        m_failed.insert(name);
        return nullptr;
    }

    LoadMeta(path + ".meta", *asset);      // optional

    m_models[name] = asset;

    return asset;
}

// Plain text, one record per line:   triangles N   |   bounds minx miny minz maxx maxy maxz   |   anchor name x y z
bool AssetManager::LoadMeta(const std::string& path, ModelAsset& asset)
{
    std::ifstream file(path.c_str());
    std::string key, name, rest;
    XMFLOAT3 point;

    if(file.fail())
    {
        return false;
    }

    while(file >> key)
    {
        if(key == "triangles")
        {
            file >> asset.triangles;
        }
        else if(key == "bounds")
        {
            file >> asset.boundsMin.x >> asset.boundsMin.y >> asset.boundsMin.z;
            file >> asset.boundsMax.x >> asset.boundsMax.y >> asset.boundsMax.z;
        }
        else if(key == "anchor")
        {
            file >> name >> point.x >> point.y >> point.z;
            asset.anchors[name] = point;
        }
        else
        {
            std::getline(file, rest);
        }
    }

    return true;
}

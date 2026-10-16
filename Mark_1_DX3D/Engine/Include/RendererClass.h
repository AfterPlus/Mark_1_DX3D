#pragma once

#include <windows.h>
#include <vector>

#include "D3dClass.h"
#include "CameraClass.h"
#include "ModelClass.h"
#include "LightClass.h"
#include "ViewpointClass.h"
#include "RenderTextureClass.h"
#include "DepthShaderClass.h"
#include "ShadowShaderClass.h"
#include "SoftShadowShaderClass.h"
#include "AssetManager.h"
#include "FoliageShaderClass.h"

const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;
const int SHADOWMAP_WIDTH = 1024;
const int SHADOWMAP_HEIGHT = 1024;
// MSAA samples per pixel for the main scene (1 = off, 2/4/8 = smoother edges, more GPU cost).
// If the GPU does not support the requested count, D3dClass falls back to the next lower one.
const int MSAA_SAMPLE_COUNT = 4;
// Most foliage copies (grass tufts ...) drawn in one frame; more are dropped.
const int MAX_FOLIAGE_INSTANCES = 16384;

// One thing to draw: which model, and where it is in the world.
// Stored as XMFLOAT4X4 (not XMMATRIX) so it is safe to keep inside std::vector.
struct RenderItem
{
    ModelClass* model;
    XMFLOAT4X4 world;
    bool castsShadow = true;        // false = left out of the shadow map (still receives shadows)
};

// Many copies of one foliage model (instanced, alpha cut-out, swaying in the wind). Foliage does not cast shadows,
// it takes the shadow of the ground it stands on.
struct FoliageBatch
{
    ModelClass* model;
    std::vector<FoliageInstance> instances;
};

struct FoliageScene
{
    std::vector<FoliageBatch> batches;
    FoliageWind wind;
};

class RendererClass
{
public:
    RendererClass();
    ~RendererClass();

    bool Initialize(int screenWidth, int screenHeight, HWND hwnd);
    void Shutdown();
    bool OnResize(int screenWidth, int screenHeight);

    // Moves the shadow-casting light and re-aims it. The game calls this
    // (later: every frame so the shadow follows the car).
    void SetLight(XMFLOAT3 position, XMFLOAT3 lookAt);
    void SetLightProjection(float fieldOfView, float nearPlane, float farPlane);

    // Shadow passes + the lit scene pass (+ the foliage, if given). Calls BeginScene. Does NOT present,
    // so the HUD can still draw on top before EndFrame().
    bool RenderScene(CameraClass* camera, const std::vector<RenderItem>& items, const FoliageScene* foliage = nullptr);
    void EndFrame();

    // The Hud needs the device / context / ortho matrix.
    D3dClass* GetDirect3D() { return m_Direct3D; }

    // Shared models + the palette texture.
    AssetManager* GetAssets() { return m_Assets; }

private:
    bool RenderDepthToTexture(const std::vector<RenderItem>& items);
    bool RenderBlackAndWhiteShadows(CameraClass* camera, const std::vector<RenderItem>& items);
    void UpdateShadowBias();
    bool RenderFoliage(CameraClass* camera, const FoliageScene& foliage);

private:
    D3dClass* m_Direct3D;
    AssetManager* m_Assets;
    LightClass* m_ProjectionLight;
    ViewPointClass* m_ViewPoint;
    RenderTextureClass* m_RenderTexture;
    RenderTextureClass* m_BlackWhiteRenderTexture;
    DepthShaderClass* m_DepthShader;
    ShadowShaderClass* m_ShadowShader;
    SoftShadowShaderClass* m_SoftShadowShader;
    FoliageShaderClass* m_FoliageShader;
    std::vector<FoliageInstance> m_foliageInstances;     // every batch's instances back to back, uploaded once per frame
    float m_shadowMapBias;      // in shadow-map depth (z/w) units, derived from m_shadowBiasMeters
    float m_shadowBiasMeters;   // how far (world metres) a surface may be behind the shadow map and still count as lit
    float m_lightNear;
    float m_lightFar;
    float m_lightDistance;      // light position -> look-at point
};
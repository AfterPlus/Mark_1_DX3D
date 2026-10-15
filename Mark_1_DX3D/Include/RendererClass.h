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

const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;
const int SHADOWMAP_WIDTH = 1024;
const int SHADOWMAP_HEIGHT = 1024;
// MSAA samples per pixel for the main scene (1 = off, 2/4/8 = smoother edges, more GPU cost).
// If the GPU does not support the requested count, D3dClass falls back to the next lower one.
const int MSAA_SAMPLE_COUNT = 4;

// One thing to draw: which model, and where it is in the world.
// Stored as XMFLOAT4X4 (not XMMATRIX) so it is safe to keep inside std::vector.
struct RenderItem
{
    ModelClass* model;
    XMFLOAT4X4 world;
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

    // Shadow passes + the lit scene pass. Calls BeginScene. Does NOT present,
    // so the HUD can still draw on top before EndFrame().
    bool RenderScene(CameraClass* camera, const std::vector<RenderItem>& items);
    void EndFrame();

    // The Hud needs the device / context / ortho matrix.
    D3dClass* GetDirect3D() { return m_Direct3D; }

private:
    bool RenderDepthToTexture(const std::vector<RenderItem>& items);
    bool RenderBlackAndWhiteShadows(CameraClass* camera, const std::vector<RenderItem>& items);

private:
    D3dClass* m_Direct3D;
    LightClass* m_ProjectionLight;
    ViewPointClass* m_ViewPoint;
    RenderTextureClass* m_RenderTexture;
    RenderTextureClass* m_BlackWhiteRenderTexture;
    DepthShaderClass* m_DepthShader;
    ShadowShaderClass* m_ShadowShader;
    SoftShadowShaderClass* m_SoftShadowShader;
    float m_shadowMapBias;
};
#pragma once

#include <windows.h>
#include <D3dClass.h>
#include <CameraClass.h>
#include <ColorShaderClass.h>
#include <ModelClass.h>
#include "InputClass.h"
#include "textureshaderclass.h"
#include "LightShaderClass.h"
#include "LightClass.h"
#include "SpriteClass.h"
#include "TimerClass.h"
#include "FontShaderClass.h"
#include "FontClass.h"
#include "TextClass.h"
#include "FpsClass.h"
#include "Multitextureshaderclass.h"
#include "lightmapshaderclass.h"
#include "AlphaMapShaderClass.h"
#include "NormalMapShaderClass.h"
#include "ModelListClass.h"
#include "PositionClass.h"
#include "FrustumClass.h"

// GLOBALS
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

class Application
{
public:
    Application();
    Application(const Application&);
    ~Application();

    bool Initialize(int screenWidth, int screenHeight, HWND hwnd);
    void Shutdown();
    bool Frame(InputClass*);
    bool OnResize(int screenWidth, int screenHeight);

private:

    bool Render();
    bool UpdateFps();
    bool UpdateRenderCountString(int);
    bool UpdateMouseStrings(int, int, bool);

    D3dClass* m_Direct3D;
    CameraClass* m_Camera;
    ModelClass* m_Model;
    ColorShaderClass* m_ColorShader;
    TextureShaderClass* m_TextureShader;
    LightShaderClass* m_LightShader;
    LightClass* m_Light;
    MultiTextureShaderClass* m_MultiTextureShader;
    LightMapShaderClass* m_LightMapShader;
    AlphaMapShaderClass* m_AlphaMapShader;
    NormalMapShaderClass* m_NormalMapShader;
    ModelClass* m_AlphaModel;
    ModelClass* m_NormalModel;


    LightClass* m_Lights;
    int m_numLights;

    int m_screenWidth, m_screenHeight;
    SpriteClass* m_Sprite;
    TimerClass* m_Timer;

    FontShaderClass* m_FontShader;
    FontClass* m_Font;
    FpsClass* m_Fps;
    TextClass* m_FpsString;
    TextClass* m_TextString1;
    TextClass* m_TextString2;
    TextClass* m_MouseStrings;
    TextClass* m_RenderCountString;
    ModelListClass* m_ModelList;
    PositionClass* m_Position;
    FrustumClass* m_Frustum;
    XMFLOAT4X4 m_baseViewMatrix;
    int m_previousFps;
};

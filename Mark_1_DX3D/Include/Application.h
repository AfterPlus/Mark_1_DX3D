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
#include "RenderTextureClass.h"
#include "DisplayPlaneClass.h"

// GLOBALS
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

// The screen edge a HUD widget is docked to. A widget collapses toward its edge when it is minimized.
enum DockEdge
{
    DOCK_LEFT,
    DOCK_RIGHT,
    DOCK_TOP,
    DOCK_BOTTOM
};

// The HUD widgets, each one a titled group of text lines with a minimize button.
enum
{
    WIDGET_FPS,
    WIDGET_RENDER,
    WIDGET_MOUSE,
    WIDGET_MESSAGES,
    WIDGET_COUNT
};

const int WIDGET_MAX_LINES = 3;

struct HudWidget
{
    const char* titleText;
    DockEdge edge;
    int anchor;                         // Offset along the docked edge in pixels (y for left/right, x for top/bottom).
    int width;                          // Width of the expanded widget in pixels.
    int lineCount;
    TextClass* lines[WIDGET_MAX_LINES]; // The text lines shown while the widget is expanded (owned by the Application).
    bool minimized;
    TextClass title;                    // The widget title, only shown while expanded.
    TextClass button;                   // The minimize / restore button, always shown.
    int buttonX, buttonY, buttonWidth;  // Button hit area, updated by the layout.
    int lineX, lineY;                   // Position of the first line, updated by the layout.
};

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
    bool RenderSceneToTexture();
    bool UpdateFps();
    bool UpdateRenderCountString(int);
    bool UpdateMouseStrings(int, int, bool);

    bool InitializeWidgets();
    void ShutdownWidgets();
    bool LayoutWidgets();
    bool HandleWidgetClick(int, int);
    bool RenderWidgets(XMMATRIX, XMMATRIX, XMMATRIX);
    bool RenderText(TextClass*, XMMATRIX, XMMATRIX, XMMATRIX);
    void GetLinePosition(int, int, int&, int&);

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
    RenderTextureClass* m_RenderTexture;
    DisplayPlaneClass* m_DisplayPlane;
    float m_cubeRotation;
    HudWidget m_Widgets[WIDGET_COUNT];
    bool m_mouseWasDown;
    bool m_widgetsInitialized;
    XMFLOAT4X4 m_baseViewMatrix;
    int m_previousFps;
};

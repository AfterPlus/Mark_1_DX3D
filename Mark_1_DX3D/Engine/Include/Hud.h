#pragma once

#include <windows.h>
#include "D3dClass.h"
#include "CameraClass.h"
#include "InputClass.h"
#include "FontShaderClass.h"
#include "FontClass.h"
#include "TextClass.h"
#include "FpsClass.h"

// --- Moved verbatim from Application.h lines 38-71 ---
enum DockEdge
{
    DOCK_LEFT,
    DOCK_RIGHT,
    DOCK_TOP,
    DOCK_BOTTOM
};

enum
{
    WIDGET_FPS,
    WIDGET_MOUSE,
    WIDGET_MESSAGES,
    WIDGET_COUNT
};

const int WIDGET_MAX_LINES = 3;

struct HudWidget
{
    const char* titleText;
    DockEdge edge;
    int anchor;
    int width;
    int lineCount;
    TextClass* lines[WIDGET_MAX_LINES];
    bool minimized;
    TextClass title;
    TextClass button;
    int buttonX, buttonY, buttonWidth;
    int lineX, lineY;
};
// -----------------------------------------------------

class Hud
{
public:
    Hud();
    ~Hud();

    // direct3D is borrowed from the Renderer, the Hud does not own it.
    bool Initialize(D3dClass* direct3D, int screenWidth, int screenHeight, HWND hwnd);
    void Shutdown();

    bool Frame(InputClass* input);   // widget clicks + text updates
    bool Render();                   // the whole 2D pass
    bool OnResize(int screenWidth, int screenHeight);

    // Sets one of the three lines of the info widget (0..2). Cheap to call every frame: nothing is rebuilt unless the text changed.
    bool SetMessage(int index, const char* text);

private:
    bool UpdateFps();
    bool UpdateMouseStrings(int, int, bool);

    bool InitializeWidgets();
    void ShutdownWidgets();
    bool LayoutWidgets();
    bool HandleWidgetClick(int, int);
    bool RenderWidgets(XMMATRIX, XMMATRIX, XMMATRIX);
    bool RenderText(TextClass*, XMMATRIX, XMMATRIX, XMMATRIX);
    void GetLinePosition(int, int, int&, int&);

private:
    D3dClass* m_Direct3D;            // not owned
    int m_screenWidth, m_screenHeight;

    FontShaderClass* m_FontShader;
    FontClass* m_Font;
    FpsClass* m_Fps;
    TextClass* m_FpsString;
    TextClass* m_TextString1;
    TextClass* m_TextString2;
    TextClass* m_TextString3;
    char m_messageText[3][32];
    TextClass* m_MouseStrings;

    HudWidget m_Widgets[WIDGET_COUNT];
    bool m_mouseWasDown;
    bool m_widgetsInitialized;
    XMFLOAT4X4 m_baseViewMatrix;
    int m_previousFps;
};

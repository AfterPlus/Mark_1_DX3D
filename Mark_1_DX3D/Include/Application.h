#pragma once

#define DIRECTINPUT_VERSION 0x0800

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <windows.h>
#include <dinput.h>
#include <D3dClass.h>
#include <CameraClass.h>
#include <ColorShaderClass.h>
#include <ModelClass.h>
#include "textureshaderclass.h"
#include "LightShaderClass.h"
#include "LightClass.h"

// GLOBALS
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

class InputClass;

class Application
{
public:
    Application();
    Application(const Application&);
    ~Application();

    bool Initialize(int screenWidth, int screenHeight, HWND hwnd);
    void Shutdown();
    bool Frame();
    bool OnResize(int screenWidth, int screenHeight);

private:

    bool Render(float);
    bool ReadMouse();

    D3dClass* m_Direct3D;
    CameraClass* m_Camera;
    ModelClass* m_Model;
    ColorShaderClass* m_ColorShader;
    TextureShaderClass* m_TextureShader;
    LightShaderClass* m_LightShader;
    LightClass* m_Light;
    
    LightClass* m_Lights;
    int m_numLights;

    IDirectInput8* m_directInput;
    IDirectInputDevice8* m_mouse;
    DIMOUSESTATE m_mouseState;
    int m_screenWidth, m_screenHeight;
    LONG m_mouseX, m_mouseY;
    POINT m_mousePosition;
};

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "Application.h"
#include "InputClass.h"

// GLOBALS //
/////////////
const bool START_FULLSCREEN = false;

class SystemClass
{
public:
    SystemClass();
    SystemClass(const SystemClass&);
    ~SystemClass();

    bool Initialize();
    void Shutdown();
    void Run();

    LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
    bool Frame();
    void InitializeWindows(int&, int&);
    void ShutdownWindows();
    void ToggleFullscreen();
    void ComputeWindowLayout(bool fullscreen, DWORD& style, DWORD& exStyle, int& x, int& y, int& width, int& height, int& clientWidth, int& clientHeight);

private:
    static const int WINDOWED_WIDTH = 1280;
    static const int WINDOWED_HEIGHT = 720;

    LPCWSTR m_applicationName;
    HINSTANCE m_hinstance;
    HWND m_hwnd;
    bool m_fullscreen;

    InputClass* m_Input;
    Application* m_Application;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static SystemClass* ApplicationHandle = 0;

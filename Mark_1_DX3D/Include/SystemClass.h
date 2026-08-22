#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "Application.h"
#include "InputClass.h"

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

private:
    LPCWSTR m_applicationName;
    HINSTANCE m_hinstance;
    HWND m_hwnd;

    InputClass* m_Input;
    Application* m_Application;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static SystemClass* ApplicationHandle = 0;

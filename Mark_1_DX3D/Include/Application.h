#pragma once

#include <windows.h>
#include "InputClass.h"
#include "TimerClass.h"
#include "RendererClass.h"
#include "Hud.h"
#include "Game.h"

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
    RendererClass* m_Renderer;
    Hud* m_Hud;
    Game* m_Game;
    TimerClass* m_Timer;
};

#pragma once

#include <windows.h>
#include "InputClass.h"
#include "RendererClass.h"

// Everything the Application needs to know about "a game".
// The demo scene and (soon) the driving game both implement this.
class Game
{
public:
    virtual ~Game() {}

    // The renderer is borrowed (Application owns it). Games load their models from it.
    virtual bool Initialize(RendererClass* renderer, HWND hwnd) = 0;
    virtual void Shutdown() = 0;

    // dt = seconds since last frame.
    virtual bool Update(float dt, InputClass* input) = 0;

    // Build the list of RenderItems and call renderer->RenderScene(...).
    virtual bool Render() = 0;
};

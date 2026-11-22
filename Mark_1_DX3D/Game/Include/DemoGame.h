#pragma once

#include <vector>
#include "Game.h"
#include "CameraClass.h"
#include "PositionClass.h"
#include "ModelClass.h"

// The phase 1 scene (ground, cube, sphere), moved out of Application.
// Exists so we can prove the refactor changed nothing. It gets deleted when the driving game starts.
class DemoGame : public Game
{
public:
    DemoGame();
    ~DemoGame();

    bool Initialize(RendererClass* renderer, HWND hwnd) override;
    void Shutdown() override;
    bool Update(float dt, InputClass* input) override;
    bool Render() override;

private:
    RendererClass* m_Renderer;       // not owned
    CameraClass* m_Camera;
    PositionClass* m_Position;
    ModelClass* m_GroundModel;
    ModelClass* m_CubeModel;
    ModelClass* m_SphereModel;
    std::vector<RenderItem> m_Items;
};

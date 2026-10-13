#pragma once

#include <vector>
#include "Game.h"
#include "CameraClass.h"
#include "ModelClass.h"
#include "Car.h"

class DrivingGame : public Game
{
public:
    DrivingGame();
    ~DrivingGame();

    bool Initialize(RendererClass* renderer, HWND hwnd) override;
    void Shutdown() override;
    bool Update(float dt, InputClass* input) override;
    bool Render() override;

private:
    bool RebuildGround(float centerX, float centerZ);
    void UpdateCamera();
    void UpdateLight();

private:
    RendererClass* m_Renderer;          // not owned
    CameraClass* m_Camera;
    ModelClass* m_CarModel;
    ModelClass* m_GroundModel;
    Car m_Car;

    std::vector<RenderItem> m_Items;    // [0] = ground, [1] = car
    std::vector<ModelClass::MeshVertex> m_GroundVertices;
    float m_groundCenterX, m_groundCenterZ;
};

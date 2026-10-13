#pragma once

#include <vector>
#include "Game.h"
#include "CameraClass.h"
#include "ModelClass.h"
#include "Car.h"
#include "Road.h"

// Drift Horizon: an endless winding road, a car that steers across it and slides in the bends.
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
    bool RebuildMeshes(float meshStart);
    void AddStrip(std::vector<ModelClass::MeshVertex>& vertices, float meshStart, float lateralA, float lateralB, int lateralCells, float tile);
    void BuildDrawList(float meshStart);
    void Restart();
    void UpdateCamera(float dt);
    void UpdateLight();

private:
    RendererClass* m_Renderer;          // not owned
    CameraClass* m_Camera;
    ModelClass* m_CarModel;
    ModelClass* m_PostModel;
    ModelClass* m_RoadModel;
    ModelClass* m_GroundModel;

    Road m_Road;
    Car m_Car;

    std::vector<RenderItem> m_Items;
    std::vector<ModelClass::MeshVertex> m_RoadVertices;
    std::vector<ModelClass::MeshVertex> m_GroundVertices;

    float m_meshStart;                  // road distance where the current road / ground meshes begin
    bool m_meshBuilt;
    float m_cameraYaw;                  // radians, smoothed
    bool m_restartWasDown;              // R key state last frame, so one press = one restart
};

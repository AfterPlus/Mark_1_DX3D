#pragma once

#include <vector>
#include "Game.h"
#include "CameraClass.h"
#include "ModelClass.h"
#include "Car.h"
#include "Road.h"
#include "DriftScore.h"
#include "Foliage.h"

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
    bool GetHudLine(int index, char* text, int size) override;

private:
    bool RebuildMeshes(float meshStart);
    void AddStrip(std::vector<ModelClass::MeshVertex>& vertices, float meshStart, float lateralA, float lateralB, int lateralCells, float tile, bool fullWidthTexture = false);
    void SurfacePoint(float s, float lateral, XMFLOAT3& position, XMFLOAT3& normal) const;
    float GroundMeshHeight(float s, float lateral) const;
    void BuildDrawList(float meshStart);
    void Restart();
    void UpdateCamera(float dt);
    void UpdateLight();
    float GetCarHeight() const;

private:
    RendererClass* m_Renderer;          // not owned
    CameraClass* m_Camera;
    ModelClass* m_CarModel;
    bool m_carIsAsset;                  // true: m_CarModel belongs to the AssetManager and is drawn unscaled
    ModelClass* m_PostModel;
    ModelClass* m_RoadModel;
    ModelClass* m_GroundModel;

    Road m_Road;
    Car m_Car;
    DriftScore m_Score;
    Foliage m_Foliage;

    std::vector<RenderItem> m_Items;
    std::vector<ModelClass::MeshVertex> m_RoadVertices;
    std::vector<ModelClass::MeshVertex> m_GroundVertices;

    float m_meshStart;                  // road distance where the current road / ground meshes begin
    bool m_meshBuilt;
    float m_cameraYaw;                  // radians, smoothed
    bool m_restartWasDown;              // R key state last frame, so one press = one restart
};

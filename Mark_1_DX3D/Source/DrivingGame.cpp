#include "DrivingGame.h"
#include "MeshBuilder.h"

// Ground: GROUND_CELLS x GROUND_CELLS cells of GROUND_CELL metres, centred on the car.
static const float GROUND_CELL = 12.0f;
static const int   GROUND_CELLS = 40;                                   // 480 m square
static const float GROUND_TILE = 6.0f;                                  // metres per texture repeat (divides GROUND_CELL)
static const int   GROUND_MAX_VERTICES = GROUND_CELLS * GROUND_CELLS * 6;

// Car box: cube.txt is 2 x 2 x 2, so these are half extents -> 1.8 m wide, 1.0 m high, 4.0 m long.
static const float CAR_HALF_WIDTH = 0.9f;
static const float CAR_HALF_HEIGHT = 0.5f;
static const float CAR_HALF_LENGTH = 2.0f;

// Follow camera.
static const float CAMERA_DISTANCE = 9.0f;
static const float CAMERA_HEIGHT = 4.5f;
static const float CAMERA_PITCH_DEGREES = 14.0f;

// Shadow "sun": far away and high so the light direction hardly changes across the screen (see 0.B).
static const XMFLOAT3 LIGHT_OFFSET = XMFLOAT3(-35.0f, 70.0f, -35.0f);

DrivingGame::DrivingGame()
{
    m_Renderer = nullptr;
    m_Camera = nullptr;
    m_CarModel = nullptr;
    m_GroundModel = nullptr;
    m_groundCenterX = 0.0f;
    m_groundCenterZ = 0.0f;
}

DrivingGame::~DrivingGame()
{
}

bool DrivingGame::Initialize(RendererClass* renderer, HWND hwnd)
{
    char modelFilename[128];
    char textureFilename[128];
    RenderItem item;
    bool result;

    m_Renderer = renderer;

    ID3D11Device* device = m_Renderer->GetDirect3D()->GetDevice();
    ID3D11DeviceContext* context = m_Renderer->GetDirect3D()->GetDeviceContext();

    m_Camera = new CameraClass;

    // Car model (static, from file).
    strcpy_s(modelFilename, "Resources/cube.txt");
    strcpy_s(textureFilename, "Resources/stone01.tga");

    m_CarModel = new ModelClass;

    result = m_CarModel->Initialize(device, context, modelFilename, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the car model object.", L"Error", MB_OK);
        return false;
    }

    // Ground model (dynamic, filled by RebuildGround).
    strcpy_s(textureFilename, "Resources/metal001.tga");

    m_GroundModel = new ModelClass;

    result = m_GroundModel->InitializeDynamic(device, context, GROUND_MAX_VERTICES, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
        return false;
    }

    // Narrow, far shadow light: ~54 m of shadowed area around the car (1024 px -> ~19 px per metre).
    m_Renderer->SetLightProjection(0.6f, 10.0f, 200.0f);

    m_Car.Reset(0.0f, 0.0f, 0.0f);

    // Draw list: slot 0 = ground, slot 1 = car (its matrix changes every frame).
    item.model = m_GroundModel;
    XMStoreFloat4x4(&item.world, XMMatrixIdentity());
    m_Items.push_back(item);

    item.model = m_CarModel;
    XMStoreFloat4x4(&item.world, XMMatrixIdentity());
    m_Items.push_back(item);

    if(!RebuildGround(0.0f, 0.0f))
    {
        return false;
    }

    UpdateCamera();
    UpdateLight();

    return true;
}

void DrivingGame::Shutdown()
{
    m_Items.clear();

    if(m_GroundModel)
    {
        m_GroundModel->Shutdown();
        delete m_GroundModel;
        m_GroundModel = nullptr;
    }

    if(m_CarModel)
    {
        m_CarModel->Shutdown();
        delete m_CarModel;
        m_CarModel = nullptr;
    }

    if(m_Camera)
    {
        delete m_Camera;
        m_Camera = nullptr;
    }

    m_Renderer = nullptr;
}

bool DrivingGame::Update(float dt, InputClass* input)
{
    XMFLOAT3 position;
    XMMATRIX world;
    float throttle, brake, steer, snapX, snapZ;

    // Keys -> car controls.
    throttle = input->IsKeyDown(DIK_UP) ? 1.0f : 0.0f;
    brake = input->IsKeyDown(DIK_DOWN) ? 1.0f : 0.0f;
    steer = 0.0f;
    if(input->IsKeyDown(DIK_RIGHT)) { steer += 1.0f; }
    if(input->IsKeyDown(DIK_LEFT))  { steer -= 1.0f; }

    m_Car.Update(dt, throttle, brake, steer);
    position = m_Car.GetPosition();

    // Keep the ground under the car, rewriting the mesh only when the car enters a new cell.
    snapX = floorf(position.x / GROUND_CELL + 0.5f) * GROUND_CELL;
    snapZ = floorf(position.z / GROUND_CELL + 0.5f) * GROUND_CELL;

    if(snapX != m_groundCenterX || snapZ != m_groundCenterZ)
    {
        if(!RebuildGround(snapX, snapZ))
        {
            return false;
        }
    }

    // Car world matrix: scale the 2x2x2 cube into a car box, turn it, put it on the ground.
    // (Row-vector convention: matrices apply left to right.)
    world = XMMatrixScaling(CAR_HALF_WIDTH, CAR_HALF_HEIGHT, CAR_HALF_LENGTH) *
            XMMatrixRotationY(m_Car.GetHeading()) *
            XMMatrixTranslation(position.x, CAR_HALF_HEIGHT, position.z);

    XMStoreFloat4x4(&m_Items[1].world, world);

    UpdateCamera();
    UpdateLight();

    return true;
}

bool DrivingGame::Render()
{
    return m_Renderer->RenderScene(m_Camera, m_Items);
}

bool DrivingGame::RebuildGround(float centerX, float centerZ)
{
    XMFLOAT3 p[4];
    XMFLOAT2 uv[4];
    float x0, x1, z0, z1;
    int ix, iz;

    m_GroundVertices.clear();

    for(iz=0; iz<GROUND_CELLS; iz++)
    {
        for(ix=0; ix<GROUND_CELLS; ix++)
        {
            x0 = centerX + (ix - GROUND_CELLS / 2) * GROUND_CELL;
            x1 = x0 + GROUND_CELL;
            z0 = centerZ + (iz - GROUND_CELLS / 2) * GROUND_CELL;
            z1 = z0 + GROUND_CELL;

            p[0] = XMFLOAT3(x0, 0.0f, z0);
            p[1] = XMFLOAT3(x0, 0.0f, z1);
            p[2] = XMFLOAT3(x1, 0.0f, z1);
            p[3] = XMFLOAT3(x1, 0.0f, z0);

            // uv measured from the grid centre (a multiple of GROUND_TILE) -> pattern stays fixed in the world.
            uv[0] = XMFLOAT2((x0 - centerX) / GROUND_TILE, (z0 - centerZ) / GROUND_TILE);
            uv[1] = XMFLOAT2((x0 - centerX) / GROUND_TILE, (z1 - centerZ) / GROUND_TILE);
            uv[2] = XMFLOAT2((x1 - centerX) / GROUND_TILE, (z1 - centerZ) / GROUND_TILE);
            uv[3] = XMFLOAT2((x1 - centerX) / GROUND_TILE, (z0 - centerZ) / GROUND_TILE);

            MeshBuilder::AddQuad(m_GroundVertices, p, uv, XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

    m_groundCenterX = centerX;
    m_groundCenterZ = centerZ;

    return m_GroundModel->UpdateMesh(m_Renderer->GetDirect3D()->GetDeviceContext(), m_GroundVertices.data(), (int)m_GroundVertices.size());
}

void DrivingGame::UpdateCamera()
{
    XMFLOAT3 position = m_Car.GetPosition();
    XMFLOAT3 forward = m_Car.GetForward();
    float headingDegrees = m_Car.GetHeading() * (180.0f / XM_PI);

    // Behind and above the car, looking the way the car faces, tilted slightly down.
    m_Camera->SetPosition(position.x - forward.x * CAMERA_DISTANCE, CAMERA_HEIGHT, position.z - forward.z * CAMERA_DISTANCE);
    m_Camera->SetRotation(CAMERA_PITCH_DEGREES, headingDegrees, 0.0f);
}

void DrivingGame::UpdateLight()
{
    XMFLOAT3 position = m_Car.GetPosition();

    // The shadow light rides along with the car at a fixed offset.
    m_Renderer->SetLight(XMFLOAT3(position.x + LIGHT_OFFSET.x, LIGHT_OFFSET.y, position.z + LIGHT_OFFSET.z), position);
}

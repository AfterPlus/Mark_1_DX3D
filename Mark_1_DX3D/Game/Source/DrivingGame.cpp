#include "DrivingGame.h"
#include "MeshBuilder.h"

// The road / ground meshes cover [meshStart, meshStart + MESH_LENGTH] along the road, in slices of MESH_STEP metres.
// meshStart is snapped to MESH_STEP, so the mesh only changes every MESH_STEP metres driven.
static const float MESH_STEP = 6.0f;
static const float MESH_BEHIND = 40.0f;
static const float MESH_LENGTH = 420.0f;
static const int   MESH_SEGMENTS = 71;                  // MESH_LENGTH / MESH_STEP + 1

// Ground strip beside the road: endless in front, only this wide on the sides.
static const float GROUND_HALF_WIDTH = 60.0f;
static const int   GROUND_LATERAL_CELLS = 16;           // small cells keep the per-vertex lighting accurate (see guide 0.B) and the banks / drops smooth;
                                                        // they are narrowest at the road edge, where the land changes fastest
// T_RoadTexture.tga is a whole 4-lane cross-section: it spans the road's full width ONCE and repeats only along the
// road. This is how many metres of road one repeat covers (dashes are ~1/12 of it: 21 m -> a dash every ~3.5 m).
static const float ROAD_TEXTURE_LENGTH = 21.0f;
static const char* ROAD_TEXTURE_FILE = "Resources/Textures/T_RoadTexture.tga";
static const float GROUND_TILE = 12.0f;

static const int ROAD_MAX_VERTICES = MESH_SEGMENTS * 6;
static const int GROUND_MAX_VERTICES = MESH_SEGMENTS * 6 * GROUND_LATERAL_CELLS * 2;

// Roadside posts: give a sense of speed.
static const float POST_SPACING = 18.0f;
static const float POST_OFFSET = ROAD_HALF_WIDTH + 1.2f;

// Car box: cube.txt is 2 x 2 x 2, so these are half extents -> 1.8 m wide, 1.0 m high, 4.0 m long.
static const float CAR_HALF_WIDTH = 0.9f;
static const float CAR_HALF_HEIGHT = 0.5f;
static const float CAR_HALF_LENGTH = 2.0f;

// Which exported model is the player's car (Resources/Models/<name>.txt).
static const char* CAR_MODEL_NAME = "car_drifthorizon";

// Draw size of the car model (1 = as exported: 4.0 m long). Looks only; the physics box is unchanged.
static const float CAR_MODEL_SCALE = 1.5f;

// Restart (R): the car is put back on the centre of the road, right where it is, with a push forward.
static const float RESTART_SPEED = 15.0f;

// Follow camera: it follows the ROAD direction (plus a little of the car's nose), not the car's every wiggle.
static const float CAMERA_DISTANCE = 9.0f;
static const float CAMERA_HEIGHT = 4.5f;
static const float CAMERA_PITCH_DEGREES = 14.0f;
static const float CAMERA_YAW_RESPONSE = 5.0f;
static const float CAMERA_NOSE_FOLLOW = 0.35f;

// Shadow "sun": far away and high so the light direction hardly changes across the screen.
static const XMFLOAT3 LIGHT_OFFSET = XMFLOAT3(-35.0f, 70.0f, -35.0f);

// World position of the point `lateral` metres to the right of a road point (at height y).
static XMFLOAT3 OffsetFromRoad(const RoadPoint& point, float lateral, float y)
{
    return XMFLOAT3(point.x + cosf(point.heading) * lateral, y, point.z - sinf(point.heading) * lateral);
}

// Lateral position of cell edge `index` of a strip split into `cells` cells. With `denseAtRoad` the cells start narrow at
// the edge nearest the road and widen outward (quadratic spacing); otherwise they are even.
static float StripLateral(float lateralA, float lateralB, int index, int cells, bool denseAtRoad)
{
    float t = (float)index / (float)cells;

    if(!denseAtRoad)
    {
        return lateralA + (lateralB - lateralA) * t;
    }

    if(fabsf(lateralA) <= fabsf(lateralB))
    {
        return lateralA + (lateralB - lateralA) * t * t;
    }

    return lateralB + (lateralA - lateralB) * t * t;
}

static float WrapAngle(float angle)
{
    while(angle > XM_PI)  { angle -= XM_2PI; }
    while(angle < -XM_PI) { angle += XM_2PI; }

    return angle;
}

DrivingGame::DrivingGame()
{
    m_Renderer = nullptr;
    m_Camera = nullptr;
    m_CarModel = nullptr;
    m_carIsAsset = false;
    m_PostModel = nullptr;
    m_RoadModel = nullptr;
    m_GroundModel = nullptr;
    m_meshStart = 0.0f;
    m_meshBuilt = false;
    m_cameraYaw = 0.0f;
    m_restartWasDown = false;
}

DrivingGame::~DrivingGame()
{
}

bool DrivingGame::Initialize(RendererClass* renderer, HWND hwnd)
{
    char modelFilename[128];
    char textureFilename[128];
    RoadPoint point;
    bool result;

    m_Renderer = renderer;

    ID3D11Device* device = m_Renderer->GetDirect3D()->GetDevice();
    ID3D11DeviceContext* context = m_Renderer->GetDirect3D()->GetDeviceContext();

    m_Camera = new CameraClass;

    // Car: the exported low-poly model if there is one, otherwise a squashed cube.
    const ModelAsset* carAsset = m_Renderer->GetAssets()->GetModel(CAR_MODEL_NAME);

    if(carAsset)
    {
        m_CarModel = carAsset->model;
        m_carIsAsset = true;
    }
    else
    {
        strcpy_s(modelFilename, "Resources/cube.txt");
        strcpy_s(textureFilename, "Resources/ice.tga");

        m_CarModel = new ModelClass;

        result = m_CarModel->Initialize(device, context, modelFilename, textureFilename, textureFilename);
        if(!result)
        {
            MessageBox(hwnd, L"Could not initialize the car model object.", L"Error", MB_OK);
            return false;
        }
    }

    // Roadside posts (a squashed cube).
    strcpy_s(modelFilename, "Resources/cube.txt");
    strcpy_s(textureFilename, "Resources/stone01.tga");

    m_PostModel = new ModelClass;

    result = m_PostModel->Initialize(device, context, modelFilename, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the post model object.", L"Error", MB_OK);
        return false;
    }

    // Road surface (dynamic).
    strcpy_s(textureFilename, ROAD_TEXTURE_FILE);

    m_RoadModel = new ModelClass;

    result = m_RoadModel->InitializeDynamic(device, context, ROAD_MAX_VERTICES, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the road model object.", L"Error", MB_OK);
        return false;
    }

    // Ground on both sides of the road (dynamic).
    strcpy_s(textureFilename, "_Shader/dirt01.tga");

    m_GroundModel = new ModelClass;

    result = m_GroundModel->InitializeDynamic(device, context, GROUND_MAX_VERTICES, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
        return false;
    }

    // Narrow, far shadow light: ~54 m of shadowed area around the car.
    m_Renderer->SetLightProjection(0.6f, 10.0f, 200.0f);

    // The car starts on the centre line at the start of the road, facing along it.
    m_Road.Reset(12345);
    point = m_Road.GetPoint(0.0f);
    m_Car.Reset(point.x, point.z, point.heading);
    m_Road.Update(0.0f);

    m_cameraYaw = point.heading;

    m_meshStart = floorf((m_Car.GetS() - MESH_BEHIND) / MESH_STEP) * MESH_STEP;

    if(!RebuildMeshes(m_meshStart))
    {
        return false;
    }

    BuildDrawList(m_meshStart);
    UpdateCamera(1.0f);
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

    if(m_RoadModel)
    {
        m_RoadModel->Shutdown();
        delete m_RoadModel;
        m_RoadModel = nullptr;
    }

    if(m_PostModel)
    {
        m_PostModel->Shutdown();
        delete m_PostModel;
        m_PostModel = nullptr;
    }

    if(m_CarModel)
    {
        // Asset models belong to the AssetManager.
        if(!m_carIsAsset)
        {
            m_CarModel->Shutdown();
            delete m_CarModel;
        }

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
    CarControls controls;
    float meshStart;

    // Keys -> car controls.
    controls.throttle = input->IsKeyDown(DIK_W) ? 1.0f : 0.0f;
    controls.brake = input->IsKeyDown(DIK_S) ? 1.0f : 0.0f;
    controls.handbrake = input->IsKeyDown(DIK_SPACE) ? 1.0f : 0.0f;
    controls.steer = 0.0f;
    if(input->IsKeyDown(DIK_D)) { controls.steer += 1.0f; }
    if(input->IsKeyDown(DIK_A))  { controls.steer -= 1.0f; }

    // R: restart. Only on the press, not while the key is held.
    bool restartDown = input->IsKeyDown(DIK_R);
    if(restartDown && !m_restartWasDown)
    {
        Restart();
    }
    m_restartWasDown = restartDown;

    // The road needs to exist ahead of the car before the car looks at it.
    m_Road.Update(m_Car.GetS());

    m_Car.Update(dt, controls, m_Road);

    // Drift score: the slide angle of the car, its speed, and whether it hit a wall.
    m_Score.Update(dt, m_Car.GetSlipAngle(), m_Car.GetSpeed(), !m_Car.IsOffRoad());
    if(m_Car.ConsumeWallHit())
    {
        m_Score.Crash();
    }

    // The car has moved on: make sure the road still exists in front of it, then re-place the meshes if needed.
    m_Road.Update(m_Car.GetS());

    meshStart = floorf((m_Car.GetS() - MESH_BEHIND) / MESH_STEP) * MESH_STEP;

    if(meshStart != m_meshStart)
    {
        m_meshStart = meshStart;

        if(!RebuildMeshes(m_meshStart))
        {
            return false;
        }
    }

    BuildDrawList(m_meshStart);
    UpdateCamera(dt);
    UpdateLight();

    return true;
}

void DrivingGame::Restart()
{
    RoadPoint point = m_Road.GetPoint(m_Car.GetS());

    // A restart loses the drift chain that was running (the banked total stays).
    m_Score.Crash();

    // Centre line, facing along the road, already rolling.
    m_Car.Reset(point.x, point.z, point.heading, RESTART_SPEED, m_Car.GetS());

    // Snap the camera behind the car instead of swinging round to it.
    m_cameraYaw = point.heading;
}

bool DrivingGame::GetHudLine(int index, char* text, int size)
{
    switch(index)
    {
        case 0:
        {
            sprintf_s(text, size, "Speed %d km/h  G%d", (int)(m_Car.GetSpeed() * 3.6f + 0.5f), m_Car.GetGear());
            return true;
        }

        case 1:
        {
            if(m_Score.IsChainActive())
            {
                sprintf_s(text, size, "Drift %d x%.1f", m_Score.GetChain(), m_Score.GetMultiplier());
            }
            else
            {
                sprintf_s(text, size, "Drift -");
            }

            return true;
        }

        case 2:
        {
            sprintf_s(text, size, "Score %d", m_Score.GetTotal());
            return true;
        }
    }

    return false;
}

bool DrivingGame::Render()
{
    return m_Renderer->RenderScene(m_Camera, m_Items);
}

// Writes the road and ground meshes for the stretch of road starting at meshStart.
bool DrivingGame::RebuildMeshes(float meshStart)
{
    ID3D11DeviceContext* context = m_Renderer->GetDirect3D()->GetDeviceContext();

    m_RoadVertices.clear();
    m_GroundVertices.clear();

    AddStrip(m_RoadVertices, meshStart, -ROAD_HALF_WIDTH, ROAD_HALF_WIDTH, 1, ROAD_TEXTURE_LENGTH, true);
    AddStrip(m_GroundVertices, meshStart, -GROUND_HALF_WIDTH, -ROAD_HALF_WIDTH, GROUND_LATERAL_CELLS, GROUND_TILE);
    AddStrip(m_GroundVertices, meshStart, ROAD_HALF_WIDTH, GROUND_HALF_WIDTH, GROUND_LATERAL_CELLS, GROUND_TILE);

    m_meshBuilt = true;

    if(!m_RoadModel->UpdateMesh(context, m_RoadVertices.data(), (int)m_RoadVertices.size()))
    {
        return false;
    }

    return m_GroundModel->UpdateMesh(context, m_GroundVertices.data(), (int)m_GroundVertices.size());
}

// One band of the road, from lateralA to lateralB metres right of the centre line, split into lateralCells cells.
void DrivingGame::AddStrip(std::vector<ModelClass::MeshVertex>& vertices, float meshStart, float lateralA, float lateralB, int lateralCells, float tile, bool fullWidthTexture)
{
    XMFLOAT3 p[4];
    XMFLOAT2 uv[4];
    RoadPoint a, b;
    XMFLOAT3 n[4];
    float s0, s1, l0, l1, uvBase, u0, u1;
    int segment, cell;

    // Texture coordinates are measured from a multiple of the tile length, so the pattern stays glued to the road
    // while meshStart moves along.
    uvBase = floorf(meshStart / tile) * tile;

    for(segment=0; segment<MESH_SEGMENTS; segment++)
    {
        s0 = meshStart + segment * MESH_STEP;
        s1 = s0 + MESH_STEP;

        a = m_Road.GetPoint(s0);
        b = m_Road.GetPoint(s1);

        for(cell=0; cell<lateralCells; cell++)
        {
            l0 = StripLateral(lateralA, lateralB, cell, lateralCells, !fullWidthTexture);
            l1 = StripLateral(lateralA, lateralB, cell + 1, lateralCells, !fullWidthTexture);

            SurfacePoint(s0, l0, p[0], n[0]);
            SurfacePoint(s0, l1, p[1], n[1]);
            SurfacePoint(s1, l1, p[2], n[2]);
            SurfacePoint(s1, l0, p[3], n[3]);

            if(fullWidthTexture)
            {
                // The picture spans the strip once, left edge to right edge (inset a hair so the wrap-around
                // filtering does not pull in the opposite edge).
                u0 = 0.002f + 0.996f * ((l0 - lateralA) / (lateralB - lateralA));
                u1 = 0.002f + 0.996f * ((l1 - lateralA) / (lateralB - lateralA));
            }
            else
            {
                u0 = l0 / tile;
                u1 = l1 / tile;
            }

            uv[0] = XMFLOAT2(u0, (s0 - uvBase) / tile);
            uv[1] = XMFLOAT2(u1, (s0 - uvBase) / tile);
            uv[2] = XMFLOAT2(u1, (s1 - uvBase) / tile);
            uv[3] = XMFLOAT2(u0, (s1 - uvBase) / tile);

            MeshBuilder::AddQuad(vertices, p, uv, n);
        }
    }
}

// The road / ground surface at distance s and `lateral` metres right of the centre line: its world position and its
// upward normal (from the slope along the road and across it).
void DrivingGame::SurfacePoint(float s, float lateral, XMFLOAT3& position, XMFLOAT3& normal) const
{
    static const float EPSILON = 0.5f;
    XMFLOAT3 alongA, alongB, acrossA, acrossB;
    XMVECTOR along, across;

    position = OffsetFromRoad(m_Road.GetPoint(s), lateral, m_Road.GetSurfaceHeight(s, lateral));

    alongA = OffsetFromRoad(m_Road.GetPoint(s - EPSILON), lateral, m_Road.GetSurfaceHeight(s - EPSILON, lateral));
    alongB = OffsetFromRoad(m_Road.GetPoint(s + EPSILON), lateral, m_Road.GetSurfaceHeight(s + EPSILON, lateral));
    acrossA = OffsetFromRoad(m_Road.GetPoint(s), lateral - EPSILON, m_Road.GetSurfaceHeight(s, lateral - EPSILON));
    acrossB = OffsetFromRoad(m_Road.GetPoint(s), lateral + EPSILON, m_Road.GetSurfaceHeight(s, lateral + EPSILON));

    along = XMVectorSubtract(XMLoadFloat3(&alongB), XMLoadFloat3(&alongA));
    across = XMVectorSubtract(XMLoadFloat3(&acrossB), XMLoadFloat3(&acrossA));

    // forward x right is the upward normal (the flip below is only a safety net).
    XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(along, across)));
    if(normal.y < 0.0f)
    {
        normal = XMFLOAT3(-normal.x, -normal.y, -normal.z);
    }
}

// Height of the road under the car (the road is level across its width).
float DrivingGame::GetCarHeight() const
{
    return m_Road.GetHeight(m_Car.GetS());
}

// Everything that is drawn this frame: road, ground, the car and the roadside posts.
void DrivingGame::BuildDrawList(float meshStart)
{
    RenderItem item;
    RoadPoint point;
    XMFLOAT3 position;
    XMMATRIX world, tilt;
    float s, firstPost, carY, grade, relative, pitch, roll;
    int side;

    m_Items.clear();

    item.model = m_RoadModel;
    XMStoreFloat4x4(&item.world, XMMatrixIdentity());
    m_Items.push_back(item);

    item.model = m_GroundModel;
    m_Items.push_back(item);

    // Car: wherever the physics put it, facing the way the physics turned it, sitting on the road.
    position = m_Car.GetPosition();
    carY = GetCarHeight();

    // Tilt it with the hill: nose up / down by the slope along its nose, and leaning by the slope across it
    // (when it is sideways to the road in a drift).
    point = m_Road.GetPoint(m_Car.GetS());
    grade = m_Road.GetGrade(m_Car.GetS());
    relative = m_Car.GetHeading() - point.heading;
    pitch = -atanf(grade * cosf(relative));
    roll = -atanf(grade * sinf(relative));
    tilt = XMMatrixRotationZ(roll) * XMMatrixRotationX(pitch);

    if(m_carIsAsset)
    {
        // Authored at real size with its origin on the ground in the middle of the car.
        world = XMMatrixScaling(CAR_MODEL_SCALE, CAR_MODEL_SCALE, CAR_MODEL_SCALE) * tilt *
                XMMatrixRotationY(m_Car.GetHeading()) * XMMatrixTranslation(position.x, carY, position.z);
    }
    else
    {
        world = XMMatrixScaling(CAR_HALF_WIDTH, CAR_HALF_HEIGHT, CAR_HALF_LENGTH) *
                XMMatrixTranslation(0.0f, CAR_HALF_HEIGHT, 0.0f) * tilt *
                XMMatrixRotationY(m_Car.GetHeading()) *
                XMMatrixTranslation(position.x, carY, position.z);
    }

    // The car does not cast a shadow.
    item.model = m_CarModel;
    XMStoreFloat4x4(&item.world, world);
    item.castsShadow = false;
    m_Items.push_back(item);
    item.castsShadow = true;

    // Posts on both sides, at fixed distances along the road.
    firstPost = ceilf(meshStart / POST_SPACING) * POST_SPACING;

    for(s=firstPost; s<meshStart + MESH_LENGTH; s+=POST_SPACING)
    {
        point = m_Road.GetPoint(s);

        for(side=-1; side<=1; side+=2)
        {
            position = OffsetFromRoad(point, (float)side * POST_OFFSET, m_Road.GetSurfaceHeight(s, (float)side * POST_OFFSET) + 0.7f);

            world = XMMatrixScaling(0.15f, 0.7f, 0.15f) *
                    XMMatrixRotationY(point.heading) *
                    XMMatrixTranslation(position.x, position.y, position.z);

            item.model = m_PostModel;
            XMStoreFloat4x4(&item.world, world);
            m_Items.push_back(item);
        }
    }
}

void DrivingGame::UpdateCamera(float dt)
{
    RoadPoint point = m_Road.GetPoint(m_Car.GetS());
    XMFLOAT3 position = m_Car.GetPosition();
    float targetYaw, blend;

    // Look along the road, with a little of the car's nose so a slide swings the view.
    targetYaw = point.heading + WrapAngle(m_Car.GetHeading() - point.heading) * CAMERA_NOSE_FOLLOW;

    blend = CAMERA_YAW_RESPONSE * dt;
    if(blend > 1.0f)
    {
        blend = 1.0f;
    }

    m_cameraYaw += WrapAngle(targetYaw - m_cameraYaw) * blend;

    m_Camera->SetPosition(position.x - sinf(m_cameraYaw) * CAMERA_DISTANCE, GetCarHeight() + CAMERA_HEIGHT, position.z - cosf(m_cameraYaw) * CAMERA_DISTANCE);
    m_Camera->SetRotation(CAMERA_PITCH_DEGREES, m_cameraYaw * (180.0f / XM_PI), 0.0f);
}

void DrivingGame::UpdateLight()
{
    XMFLOAT3 position = m_Car.GetPosition();

    position.y = GetCarHeight();

    // The shadow light rides along with the car at a fixed offset.
    m_Renderer->SetLight(XMFLOAT3(position.x + LIGHT_OFFSET.x, position.y + LIGHT_OFFSET.y, position.z + LIGHT_OFFSET.z), position);
}

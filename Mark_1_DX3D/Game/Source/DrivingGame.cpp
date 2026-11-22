#include "DrivingGame.h"
#include "MeshBuilder.h"

// The road / ground meshes cover [meshStart, meshStart + MESH_LENGTH] along the road, in slices of MESH_STEP metres.
// meshStart is snapped to MESH_STEP, so the mesh only changes every MESH_STEP metres driven.
static const float MESH_STEP = 6.0f;
static const float MESH_BEHIND = 40.0f;
static const float MESH_LENGTH = 420.0f;
static const int   MESH_SEGMENTS = 71;                  // MESH_LENGTH / MESH_STEP + 1

// Ground strip beside the road: endless in front, only this wide on the sides.
static const float GROUND_HALF_WIDTH = 100.0f;
static const int   GROUND_LATERAL_CELLS = 20;           // small cells keep the per-vertex lighting accurate (see guide 0.B) and the banks / drops smooth;
                                                        // they are narrowest at the road edge, where the land changes fastest
// T_RoadTexture.tga is a whole 4-lane cross-section: it spans the road's full width ONCE and repeats only along the
// road. This is how many metres of road one repeat covers (dashes are ~1/12 of it: 21 m -> a dash every ~3.5 m).
static const float ROAD_TEXTURE_LENGTH = 21.0f;
static const char* ROAD_TEXTURE_FILE = "Resources/Textures/T_RoadTexture.tga";
// Metres of ground one copy of the grass texture covers. Kept large so the repeats are few and far apart; the UV warp
// below bends the grid of copies so the ones that do show never line up in rows.
static const float GROUND_TILE = 30.0f;
// Ground UV warp: a few slow sine waves (wavelengths 90..330 m) shift the texture by up to ~1.6 * this many metres.
// Small enough that the texture never stretches by more than about a third anywhere.
static const float GROUND_WARP = 3.0f;
static const char* GROUND_TEXTURE_FILE = "Resources/Textures/T_GroundGrass.tga";     // made by Tools/make_ground_texture.py

static const int ROAD_MAX_VERTICES = MESH_SEGMENTS * 6;
static const int GROUND_MAX_VERTICES = MESH_SEGMENTS * 6 * GROUND_LATERAL_CELLS * 2;

// Grass is placed from a little behind the camera to this far ahead of the car (re-placed with the road meshes).
// The ground mesh reaches ~375 m ahead, so the grass stops just short of its end (and fades out before, see Foliage.cpp).
static const float GRASS_BEHIND = 25.0f;
static const float GRASS_AHEAD = 365.0f;

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
    strcpy_s(textureFilename, GROUND_TEXTURE_FILE);

    m_GroundModel = new ModelClass;

    result = m_GroundModel->InitializeDynamic(device, context, GROUND_MAX_VERTICES, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
        return false;
    }

    // Grass beside the road. Optional: without the converted models (Tools/blender/dh_export_grass.py) there is just no grass.
    m_Foliage.Initialize(m_Renderer);

    // Sky. Optional too: if the faces are missing the background stays black.
    m_Renderer->LoadSkybox("Resources/SkyBox/Day");

    // Narrow, far shadow light: ~54 m of shadowed area around the car.
    m_Renderer->SetLightProjection(0.6f, 10.0f, 200.0f);

    // The car starts on the centre line at the start of the road, facing along it.
    m_Road.Reset(12345);
    point = m_Road.GetPoint(0.0f);
    m_Car.Reset(point.x, point.z, point.heading);
    m_Road.Update(0.0f);
    m_Fence.Reset();

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

    m_Foliage.Shutdown();

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

    // Fence posts the car runs into break off and fly (they collide with the car as it is drawn).
    bool postHit = m_Fence.Update(dt, m_Car, m_Road, GetCarDrawScale() * CAR_HALF_WIDTH, GetCarDrawScale() * CAR_HALF_LENGTH,
                                  GetCarDrawScale() * 2.0f * CAR_HALF_HEIGHT, GetCarHeight(), m_meshStart);

    // Drift score: the slide angle of the car, its speed, and whether it hit a wall or a post.
    m_Score.Update(dt, m_Car.GetSlipAngle(), m_Car.GetSpeed(), !m_Car.IsOffRoad());
    if(m_Car.ConsumeWallHit() || postHit)
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
    m_Foliage.Update(dt);
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
    return m_Renderer->RenderScene(m_Camera, m_Items, m_Foliage.GetScene());
}

// Offset in metres (du across, dv along the road) added to the ground's texture coordinates at (s, lateral). It only
// depends on the place on the ground, so neighbouring quads and successive mesh rebuilds still agree at every vertex.
static void GroundUvWarp(float s, float lateral, float& du, float& dv)
{
    du = GROUND_WARP * (sinf(s * 0.067f + lateral * 0.023f) + 0.6f * sinf(s * 0.027f - lateral * 0.041f + 1.7f));
    dv = GROUND_WARP * (sinf(lateral * 0.063f - s * 0.019f + 0.4f) + 0.6f * sinf(lateral * 0.029f + s * 0.047f + 2.9f));
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

    // meshStart is MESH_BEHIND metres behind the car (give or take one MESH_STEP).
    // Roots go on the ground mesh as it is drawn (its flat triangles), not on the smooth terrain function it samples.
    m_Foliage.Rebuild(m_Road, meshStart + MESH_BEHIND - GRASS_BEHIND, meshStart + MESH_BEHIND + GRASS_AHEAD,
                      [this](float s, float lateral) { return GroundMeshHeight(s, lateral); });

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
    float s0, s1, l0, l1, uvBase, u0, u1, du[4], dv[4];
    int segment, cell, corner;

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

            for(corner=0; corner<4; corner++)
            {
                du[corner] = 0.0f;
                dv[corner] = 0.0f;
            }

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

                GroundUvWarp(s0, l0, du[0], dv[0]);
                GroundUvWarp(s0, l1, du[1], dv[1]);
                GroundUvWarp(s1, l1, du[2], dv[2]);
                GroundUvWarp(s1, l0, du[3], dv[3]);
            }

            uv[0] = XMFLOAT2(u0 + du[0] / tile, (s0 - uvBase + dv[0]) / tile);
            uv[1] = XMFLOAT2(u1 + du[1] / tile, (s0 - uvBase + dv[1]) / tile);
            uv[2] = XMFLOAT2(u1 + du[2] / tile, (s1 - uvBase + dv[2]) / tile);
            uv[3] = XMFLOAT2(u0 + du[3] / tile, (s1 - uvBase + dv[3]) / tile);

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

// Height of the ground mesh at distance s, `lateral` metres right of the centre line: the same flat triangles AddStrip
// draws (cells MESH_STEP long, StripLateral wide, each quad split from its road-side corner at s0 to its outer corner at
// s1). The smooth terrain (Road::GetSurfaceHeight) can be well above or below these triangles on hills and banks.
float DrivingGame::GroundMeshHeight(float s, float lateral) const
{
    float side, e, t, s0, a, b, l0, l1, h0, h1, h2, h3;
    int cell;

    side = lateral < 0.0f ? -1.0f : 1.0f;
    e = (fabsf(lateral) - ROAD_HALF_WIDTH) / (GROUND_HALF_WIDTH - ROAD_HALF_WIDTH);

    if(e <= 0.0f)
    {
        return m_Road.GetSurfaceHeight(s, lateral);
    }

    // Cell across: edges sit at t^2 of the strip (StripLateral, dense at the road).
    cell = (int)(sqrtf(e > 1.0f ? 1.0f : e) * GROUND_LATERAL_CELLS);
    if(cell >= GROUND_LATERAL_CELLS)
    {
        cell = GROUND_LATERAL_CELLS - 1;
    }

    t = (float)cell / GROUND_LATERAL_CELLS;
    l0 = ROAD_HALF_WIDTH + (GROUND_HALF_WIDTH - ROAD_HALF_WIDTH) * t * t;
    t = (float)(cell + 1) / GROUND_LATERAL_CELLS;
    l1 = ROAD_HALF_WIDTH + (GROUND_HALF_WIDTH - ROAD_HALF_WIDTH) * t * t;

    // Cell along: mesh slices start at multiples of MESH_STEP.
    s0 = floorf(s / MESH_STEP) * MESH_STEP;

    a = (s - s0) / MESH_STEP;                       // 0..1 along
    b = (fabsf(lateral) - l0) / (l1 - l0);          // 0..1 across

    // Corners as AddStrip makes them: p0 (s0, l0), p1 (s0, l1), p2 (s1, l1), p3 (s1, l0); triangles 0-1-2 and 0-2-3.
    h0 = m_Road.GetSurfaceHeight(s0, side * l0);
    h1 = m_Road.GetSurfaceHeight(s0, side * l1);
    h2 = m_Road.GetSurfaceHeight(s0 + MESH_STEP, side * l1);
    h3 = m_Road.GetSurfaceHeight(s0 + MESH_STEP, side * l0);

    if(b >= a)
    {
        return h0 + (h1 - h0) * b + (h2 - h1) * a;  // triangle p0 p1 p2
    }

    return h0 + (h3 - h0) * a + (h2 - h3) * b;      // triangle p0 p2 p3
}

// How much bigger the car is drawn than its CAR_HALF_* box (the exported model is that box at scale 1).
float DrivingGame::GetCarDrawScale() const
{
    return m_carIsAsset ? CAR_MODEL_SCALE : 1.0f;
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
    float carY, grade, relative, pitch, roll;

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

    // Fence posts: the standing ones and the ones knocked loose.
    m_Fence.AddDrawItems(m_Items, m_PostModel, m_Road, meshStart, meshStart + MESH_LENGTH);
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

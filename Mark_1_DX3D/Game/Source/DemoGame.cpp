#include "DemoGame.h"

DemoGame::DemoGame()
{
    m_Renderer = nullptr;
    m_Camera = nullptr;
    m_Position = nullptr;
    m_GroundModel = nullptr;
    m_CubeModel = nullptr;
    m_SphereModel = nullptr;
}

DemoGame::~DemoGame()
{
}

bool DemoGame::Initialize(RendererClass* renderer, HWND hwnd)
{
    char modelFilename[128];
    char textureFilename[128];
    RenderItem item;
    bool result;

    m_Renderer = renderer;

    ID3D11Device* device = m_Renderer->GetDirect3D()->GetDevice();
    ID3D11DeviceContext* context = m_Renderer->GetDirect3D()->GetDeviceContext();

    // Camera above and behind the ground looking down at it.
    m_Camera = new CameraClass;
    m_Camera->SetPosition(0.0f, 7.0f, -10.0f);
    m_Camera->SetRotation(35.0f, 0.0f, 0.0f);
    m_Camera->Render();

    m_Position = new PositionClass;

    // Ground.
    strcpy_s(modelFilename, "Resources/plane01.txt");
    strcpy_s(textureFilename, "Resources/metal001.tga");

    m_GroundModel = new ModelClass;

    result = m_GroundModel->Initialize(device, context, modelFilename, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
        return false;
    }

    // Cube.
    strcpy_s(modelFilename, "Resources/cube.txt");
    strcpy_s(textureFilename, "Resources/stone01.tga");

    m_CubeModel = new ModelClass;

    result = m_CubeModel->Initialize(device, context, modelFilename, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the cube model object.", L"Error", MB_OK);
        return false;
    }

    // Sphere.
    strcpy_s(modelFilename, "Resources/sphere.txt");
    strcpy_s(textureFilename, "Resources/ice.tga");

    m_SphereModel = new ModelClass;

    result = m_SphereModel->Initialize(device, context, modelFilename, textureFilename, textureFilename);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the sphere model object.", L"Error", MB_OK);
        return false;
    }

    // The scene never changes, so build the draw list once.
    item.model = m_GroundModel;
    XMStoreFloat4x4(&item.world, XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    m_Items.push_back(item);

    item.model = m_CubeModel;
    XMStoreFloat4x4(&item.world, XMMatrixTranslation(-2.0f, 2.0f, 0.0f));
    m_Items.push_back(item);

    item.model = m_SphereModel;
    XMStoreFloat4x4(&item.world, XMMatrixTranslation(2.0f, 2.0f, 0.0f));
    m_Items.push_back(item);

    return true;
}

void DemoGame::Shutdown()
{
    m_Items.clear();

    if(m_SphereModel)
    {
        m_SphereModel->Shutdown();
        delete m_SphereModel;
        m_SphereModel = nullptr;
    }

    if(m_CubeModel)
    {
        m_CubeModel->Shutdown();
        delete m_CubeModel;
        m_CubeModel = nullptr;
    }

    if(m_GroundModel)
    {
        m_GroundModel->Shutdown();
        delete m_GroundModel;
        m_GroundModel = nullptr;
    }

    if(m_Position)
    {
        delete m_Position;
        m_Position = nullptr;
    }

    if(m_Camera)
    {
        delete m_Camera;
        m_Camera = nullptr;
    }

    m_Renderer = nullptr;
}

bool DemoGame::Update(float dt, InputClass* input)
{
    float rotationY;

    // PositionClass's turn speeds are tuned for milliseconds, dt is in seconds.
    m_Position->SetFrameTime(dt * 1000.0f);

    m_Position->TurnLeft(input->IsLeftArrowPressed());
    m_Position->TurnRight(input->IsRightArrowPressed());

    m_Position->GetRotation(rotationY);
    m_Camera->SetRotation(35.0f, rotationY, 0.0f);

    return true;
}

bool DemoGame::Render()
{
    return m_Renderer->RenderScene(m_Camera, m_Items);
}

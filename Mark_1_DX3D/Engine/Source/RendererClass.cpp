#include "RendererClass.h"

RendererClass::RendererClass()
{
    m_Direct3D = nullptr;
    m_Assets = nullptr;
    m_ProjectionLight = nullptr;
    m_ViewPoint = nullptr;
    m_RenderTexture = nullptr;
    m_BlackWhiteRenderTexture = nullptr;
    m_DepthShader = nullptr;
    m_ShadowShader = nullptr;
    m_SoftShadowShader = nullptr;
    m_shadowMapBias = 0.0f;
}

RendererClass::~RendererClass()
{
}

bool RendererClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    bool result;

    // Create and initialize the Direct3D object.
    m_Direct3D = new D3dClass;

    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
        return false;
    }

    // Shared model / palette loader.
    m_Assets = new AssetManager;

    result = m_Assets->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext());
    if(!result)
    {
        MessageBox(hwnd, L"Could not load Resources/Textures/palette.tga (run Tools/make_palette.py).", L"Error", MB_OK);
        return false;
    }

    // The view point the shadow map is rendered from.
    m_ViewPoint = new ViewPointClass;

    m_ViewPoint->SetPosition(0.0f, 8.0f, -5.0f);
    m_ViewPoint->SetLookAt(0.0f, 0.0f, 0.0f);
    m_ViewPoint->SetProjectionParameters((XM_PI / 2.0f), 1.0f, 3.0f, 100.0f);
    m_ViewPoint->GenerateViewMatrix();
    m_ViewPoint->GenerateProjectionMatrix();

    // The point light that sits at the view point.
    m_ProjectionLight = new LightClass;

    m_ProjectionLight->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
    m_ProjectionLight->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_ProjectionLight->SetPosition(0.0f, 8.0f, -5.0f);

    // Shadow map (32 bit float so the depth keeps its precision).
    m_RenderTexture = new RenderTextureClass;

    result = m_RenderTexture->Initialize(m_Direct3D->GetDevice(), SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, SCREEN_DEPTH, SCREEN_NEAR, 2);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the render texture object.", L"Error", MB_OK);
        return false;
    }

    m_DepthShader = new DepthShaderClass;

    result = m_DepthShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the depth shader object.", L"Error", MB_OK);
        return false;
    }

    m_ShadowShader = new ShadowShaderClass;

    result = m_ShadowShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the shadow shader object.", L"Error", MB_OK);
        return false;
    }

    // Black and white shadow texture (8 bit, so it can be blurred).
    m_BlackWhiteRenderTexture = new RenderTextureClass;

    result = m_BlackWhiteRenderTexture->Initialize(m_Direct3D->GetDevice(), SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, SCREEN_DEPTH, SCREEN_NEAR, 1);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the black and white render texture object.", L"Error", MB_OK);
        return false;
    }

    m_SoftShadowShader = new SoftShadowShaderClass;

    result = m_SoftShadowShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the soft shadow shader object.", L"Error", MB_OK);
        return false;
    }

    // Fixes shadow acne.
    m_shadowMapBias = 0.005f;

    return true;
}

void RendererClass::Shutdown()
{
    if(m_SoftShadowShader)
    {
        m_SoftShadowShader->Shutdown();
        delete m_SoftShadowShader;
        m_SoftShadowShader = nullptr;
    }

    if(m_BlackWhiteRenderTexture)
    {
        m_BlackWhiteRenderTexture->Shutdown();
        delete m_BlackWhiteRenderTexture;
        m_BlackWhiteRenderTexture = nullptr;
    }

    if(m_ShadowShader)
    {
        m_ShadowShader->Shutdown();
        delete m_ShadowShader;
        m_ShadowShader = nullptr;
    }

    if(m_DepthShader)
    {
        m_DepthShader->Shutdown();
        delete m_DepthShader;
        m_DepthShader = nullptr;
    }

    if(m_RenderTexture)
    {
        m_RenderTexture->Shutdown();
        delete m_RenderTexture;
        m_RenderTexture = nullptr;
    }

    if(m_ProjectionLight)
    {
        delete m_ProjectionLight;
        m_ProjectionLight = nullptr;
    }

    if(m_ViewPoint)
    {
        delete m_ViewPoint;
        m_ViewPoint = nullptr;
    }

    if(m_Assets)
    {
        m_Assets->Shutdown();
        delete m_Assets;
        m_Assets = nullptr;
    }

    // D3D last: everything above was created from its device.
    if(m_Direct3D)
    {
        m_Direct3D->Shutdown();
        delete m_Direct3D;
        m_Direct3D = nullptr;
    }
}

bool RendererClass::OnResize(int screenWidth, int screenHeight)
{
    if(!m_Direct3D)
    {
        return false;
    }

    return m_Direct3D->ResizeBuffers(screenWidth, screenHeight);
}

void RendererClass::SetLight(XMFLOAT3 position, XMFLOAT3 lookAt)
{
    m_ViewPoint->SetPosition(position.x, position.y, position.z);
    m_ViewPoint->SetLookAt(lookAt.x, lookAt.y, lookAt.z);
    m_ViewPoint->GenerateViewMatrix();

    m_ProjectionLight->SetPosition(position.x, position.y, position.z);
}

void RendererClass::SetLightProjection(float fieldOfView, float nearPlane, float farPlane)
{
    m_ViewPoint->SetProjectionParameters(fieldOfView, 1.0f, nearPlane, farPlane);
    m_ViewPoint->GenerateProjectionMatrix();
}

// Old Application::Render (the 3D half).
bool RendererClass::RenderScene(CameraClass* camera, const std::vector<RenderItem>& items)
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix;
    XMFLOAT4 lightPosition;
    size_t i;
    bool result;

    // Update the camera view matrix for this frame.
    camera->Render();

    // Pass 1: scene from the light's point of view into the shadow map.
    result = RenderDepthToTexture(items);
    if(!result)
    {
        return false;
    }

    // Pass 2: shadowed scene in black and white using the shadow map.
    result = RenderBlackAndWhiteShadows(camera, items);
    if(!result)
    {
        return false;
    }

    // Pass 3: the real scene into the back buffer.
    m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    camera->GetViewMatrix(viewMatrix);
    m_Direct3D->GetProjectionMatrix(projectionMatrix);

    lightPosition = m_ProjectionLight->GetPosition();

    for(i=0; i<items.size(); i++)
    {
        worldMatrix = XMLoadFloat4x4(&items[i].world);

        items[i].model->Render(m_Direct3D->GetDeviceContext());

        result = m_SoftShadowShader->Render(m_Direct3D->GetDeviceContext(), items[i].model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
                                            items[i].model->GetTexture(0), m_BlackWhiteRenderTexture->GetShaderResourceView(),
                                            m_ProjectionLight->GetAmbientColor(), m_ProjectionLight->GetDiffuseColor(),
                                            XMFLOAT3(lightPosition.x, lightPosition.y, lightPosition.z),
                                            m_shadowMapBias);
        if(!result)
        {
            return false;
        }
    }

    return true;
}

void RendererClass::EndFrame()
{
    m_Direct3D->EndScene();
}

// Old Application::RenderDepthToTexture – the three copy-pasted blocks became one loop.
bool RendererClass::RenderDepthToTexture(const std::vector<RenderItem>& items)
{
    XMMATRIX worldMatrix, lightViewMatrix, lightProjectionMatrix;
    size_t i;
    bool result;

    m_RenderTexture->SetRenderTarget(m_Direct3D->GetDeviceContext());
    m_RenderTexture->ClearRenderTarget(m_Direct3D->GetDeviceContext(), 1.0f, 1.0f, 1.0f, 1.0f);

    m_ViewPoint->GetViewMatrix(lightViewMatrix);
    m_ViewPoint->GetProjectionMatrix(lightProjectionMatrix);

    for(i=0; i<items.size(); i++)
    {
        worldMatrix = XMLoadFloat4x4(&items[i].world);

        items[i].model->Render(m_Direct3D->GetDeviceContext());

        result = m_DepthShader->Render(m_Direct3D->GetDeviceContext(), items[i].model->GetIndexCount(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
        if(!result)
        {
            return false;
        }
    }

    // Back to the back buffer and the normal viewport.
    m_Direct3D->SetBackBufferRenderTarget();
    m_Direct3D->ResetViewport();

    return true;
}

// Old Application::RenderBlackAndWhiteShadows.
bool RendererClass::RenderBlackAndWhiteShadows(CameraClass* camera, const std::vector<RenderItem>& items)
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix, lightViewMatrix, lightProjectionMatrix;
    XMFLOAT4 lightPosition;
    size_t i;
    bool result;

    m_BlackWhiteRenderTexture->SetRenderTarget(m_Direct3D->GetDeviceContext());
    m_BlackWhiteRenderTexture->ClearRenderTarget(m_Direct3D->GetDeviceContext(), 0.0f, 0.0f, 0.0f, 1.0f);

    camera->GetViewMatrix(viewMatrix);
    m_Direct3D->GetProjectionMatrix(projectionMatrix);
    m_ViewPoint->GetViewMatrix(lightViewMatrix);
    m_ViewPoint->GetProjectionMatrix(lightProjectionMatrix);

    lightPosition = m_ProjectionLight->GetPosition();

    for(i=0; i<items.size(); i++)
    {
        worldMatrix = XMLoadFloat4x4(&items[i].world);

        items[i].model->Render(m_Direct3D->GetDeviceContext());

        result = m_ShadowShader->Render(m_Direct3D->GetDeviceContext(), items[i].model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
                                        lightViewMatrix, lightProjectionMatrix, m_RenderTexture->GetShaderResourceView(),
                                        XMFLOAT3(lightPosition.x, lightPosition.y, lightPosition.z), m_shadowMapBias);
        if(!result)
        {
            return false;
        }
    }

    m_Direct3D->SetBackBufferRenderTarget();
    m_Direct3D->ResetViewport();

    return true;
}
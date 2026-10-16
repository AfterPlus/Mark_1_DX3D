#include "RendererClass.h"
#include <algorithm>

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
    m_FoliageShader = nullptr;
    m_foliageVersion = 0;
    m_foliageUploaded = nullptr;
    m_shadowMapBias = 0.0f;
    m_shadowBiasMeters = 0.0f;
    m_lightNear = 0.0f;
    m_lightFar = 0.0f;
    m_lightDistance = 0.0f;
}

RendererClass::~RendererClass()
{
}

bool RendererClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    bool result;

    // Create and initialize the Direct3D object.
    m_Direct3D = new D3dClass;

    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, SCREEN_DEPTH, SCREEN_NEAR, MSAA_SAMPLE_COUNT);
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
    m_lightNear = 3.0f;
    m_lightFar = 100.0f;
    m_lightDistance = sqrtf(8.0f * 8.0f + 5.0f * 5.0f);
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

    m_FoliageShader = new FoliageShaderClass;

    result = m_FoliageShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the foliage shader object.", L"Error", MB_OK);
        return false;
    }

    // Fixes shadow acne. Kept in metres: small enough that the gap under the car still shadows,
    // large enough to cover a shadow-map texel's depth slope on the ground.
    m_shadowBiasMeters = 0.1f;
    UpdateShadowBias();

    return true;
}

void RendererClass::Shutdown()
{
    if(m_FoliageShader)
    {
        m_FoliageShader->Shutdown();
        delete m_FoliageShader;
        m_FoliageShader = nullptr;
    }

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

    m_lightDistance = sqrtf((lookAt.x - position.x) * (lookAt.x - position.x) +
                            (lookAt.y - position.y) * (lookAt.y - position.y) +
                            (lookAt.z - position.z) * (lookAt.z - position.z));
    UpdateShadowBias();
}

void RendererClass::SetLightProjection(float fieldOfView, float nearPlane, float farPlane)
{
    m_ViewPoint->SetProjectionParameters(fieldOfView, 1.0f, nearPlane, farPlane);
    m_ViewPoint->GenerateProjectionMatrix();

    m_lightNear = nearPlane;
    m_lightFar = farPlane;
    UpdateShadowBias();
}

// The shadow test compares perspective depth (z/w), which is not linear: one metre at distance d
// changes it by near*far / ((far-near) * d^2). Convert the metre bias at the look-at distance, so the
// same bias works whether the light is 10 m or 100 m away.
void RendererClass::UpdateShadowBias()
{
    if(m_lightDistance <= 0.0f || m_lightFar <= m_lightNear)
    {
        return;
    }

    m_shadowMapBias = m_shadowBiasMeters * (m_lightNear * m_lightFar) / ((m_lightFar - m_lightNear) * m_lightDistance * m_lightDistance);
}

// Old Application::Render (the 3D half).
bool RendererClass::RenderScene(CameraClass* camera, const std::vector<RenderItem>& items, const FoliageScene* foliage)
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

    // Pass 4: foliage on top of the lit scene (it uses the depth buffer the scene just wrote).
    if(foliage && !foliage->chunks.empty())
    {
        result = RenderFoliage(camera, *foliage);
        if(!result)
        {
            return false;
        }
    }

    return true;
}

// The instances live on the GPU and are re-uploaded only when the scene's version changes. Each frame every chunk is
// checked against the view frustum (cut off at the fade distance), and of the visible ones only the share its distance
// keeps is drawn: chunks are sorted by LOD rank, so that share is the start of the chunk's range.
bool RendererClass::RenderFoliage(CameraClass* camera, const FoliageScene& foliage)
{
    ID3D11DeviceContext* context = m_Direct3D->GetDeviceContext();
    XMMATRIX viewMatrix, projectionMatrix;
    XMFLOAT4 lightPosition;
    XMFLOAT3 cameraPosition;
    FoliageDraw draw;
    float dx, dz, distance, keep;
    size_t i;
    int currentModel;
    bool result;

    if(m_foliageUploaded != &foliage || m_foliageVersion != foliage.version)
    {
        result = m_FoliageShader->SetInstances(m_Direct3D->GetDevice(), context, foliage.instances.data(), (int)foliage.instances.size());
        if(!result)
        {
            return false;
        }

        m_foliageUploaded = &foliage;
        m_foliageVersion = foliage.version;
    }

    camera->GetViewMatrix(viewMatrix);
    m_Direct3D->GetProjectionMatrix(projectionMatrix);
    cameraPosition = camera->GetPosition();

    // Nothing is drawn past the fade distance, so that is the far plane for the test.
    m_foliageFrustum.ConstructFrustum(viewMatrix, projectionMatrix, foliage.wind.fadeEnd);

    m_foliageDraws.clear();

    for(i=0; i<foliage.chunks.size(); i++)
    {
        const FoliageChunk& chunk = foliage.chunks[i];

        if(chunk.count <= 0 || chunk.model < 0 || chunk.model >= (int)foliage.models.size() || !foliage.models[chunk.model].model)
        {
            continue;
        }

        if(!m_foliageFrustum.CheckSphere(chunk.center.x, chunk.center.y, chunk.center.z, chunk.radius))
        {
            continue;
        }

        // The nearest the chunk gets to the camera (on the ground plane, like the shader) keeps the most instances.
        dx = chunk.center.x - cameraPosition.x;
        dz = chunk.center.z - cameraPosition.z;
        distance = sqrtf(dx * dx + dz * dz) - chunk.radius;
        if(distance < 0.0f)
        {
            distance = 0.0f;
        }

        keep = FoliageKeepShare(distance, foliage.models[chunk.model].lodDistance, foliage.wind.lodMinKeep);

        draw.model = chunk.model;
        draw.first = chunk.first;
        draw.count = (int)ceilf((float)chunk.count * keep);
        draw.distance = distance;

        if(draw.count > chunk.count)
        {
            draw.count = chunk.count;
        }

        m_foliageDraws.push_back(draw);
    }

    if(m_foliageDraws.empty())
    {
        return true;
    }

    // Grouped by model (one mesh bind each), near to far inside a model so the depth test rejects more hidden blades.
    std::sort(m_foliageDraws.begin(), m_foliageDraws.end(), [](const FoliageDraw& a, const FoliageDraw& b)
    {
        return a.model != b.model ? a.model < b.model : a.distance < b.distance;
    });

    lightPosition = m_ProjectionLight->GetPosition();

    result = m_FoliageShader->Begin(context, viewMatrix, projectionMatrix, cameraPosition,
                                    XMFLOAT3(lightPosition.x, lightPosition.y, lightPosition.z),
                                    m_ProjectionLight->GetAmbientColor(), m_ProjectionLight->GetDiffuseColor(),
                                    foliage.wind, m_BlackWhiteRenderTexture->GetShaderResourceView());
    if(!result)
    {
        return false;
    }

    currentModel = -1;

    for(i=0; i<m_foliageDraws.size(); i++)
    {
        if(m_foliageDraws[i].model != currentModel)
        {
            currentModel = m_foliageDraws[i].model;

            result = m_FoliageShader->SetModel(context, foliage.models[currentModel].model, foliage.models[currentModel].lodDistance);
            if(!result)
            {
                m_FoliageShader->End(context);
                return false;
            }
        }

        m_FoliageShader->Draw(context, m_foliageDraws[i].first, m_foliageDraws[i].count);
    }

    m_FoliageShader->End(context);

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
        if(!items[i].castsShadow)
        {
            continue;
        }

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
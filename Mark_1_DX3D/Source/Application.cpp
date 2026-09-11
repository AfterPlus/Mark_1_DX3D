#include "Application.h"
#include "InputClass.h"

Application::Application()
{
    m_Direct3D = 0;
    m_directInput = 0;
    m_mouse = 0;
    m_mousePosition = { 0, 0 };
    m_Camera = 0;
    m_Model = 0;
    m_ColorShader = 0;
    m_TextureShader = 0 ;
}


Application::Application(const Application& other)
{
}


Application::~Application()
{
}


bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    char textureFilename[128];
    bool result;

    // Create and initialize new direct X object
    m_Direct3D = new D3dClass;

    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
        return false;
    }
    
    // Create the camera object.
    m_Camera = new CameraClass;

    // Set the initial position of the camera.
    m_Camera->SetPosition(0.0f, 0.0f, -5.0f);

    // Create and initialize the model object.
    m_Model = new ModelClass;

    // Set the name of the texture file that we will be loading.
    strcpy_s(textureFilename, "../Resource/dx11win10tut61_src/data/stone01.tga");

    result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), textureFilename);
    
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the color shader object.
    m_ColorShader = new ColorShaderClass;

    result = m_ColorShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the color shader object.", L"Error", MB_OK);
        return false;
    }
    
    // Create and initialize the texture shader object.
    m_TextureShader = new TextureShaderClass;

    result = m_TextureShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }

    return true;
    
}

void Application::Shutdown()
{
    
    // Release the texture shader object.
    if (m_TextureShader)
    {
        m_TextureShader->Shutdown();
        delete m_TextureShader;
        m_TextureShader = 0;
    }
    
    // Release the DirectInput mouse device.
    if (m_mouse)
    {
        m_mouse->Unacquire();
        m_mouse->Release();
        m_mouse = nullptr;
    }

    // Release the color shader object.
    if (m_ColorShader)
    {
        m_ColorShader->Shutdown();
        delete m_ColorShader;
        m_ColorShader = 0;
    }

    // Release the model object.
    if (m_Model)
    {
        m_Model->Shutdown();
        delete m_Model;
        m_Model = 0;
    }

    // Release the camera object.
    if (m_Camera)
    {
        delete m_Camera;
        m_Camera = 0;
    }

    // Release the directX object
    if (m_Direct3D)
    {
        m_Direct3D->Shutdown();
        delete m_Direct3D;
        m_Direct3D = nullptr;
    }
    return;
}


bool Application::Frame()
{
    bool result;
    
    // Render the graphics scene.
    result = Render();
    if(!result)
    {
        return false;
    }

    return true;
}

bool Application::OnResize(int screenWidth, int screenHeight)
{
    if (!m_Direct3D)
    {
        return false;
    }

    return m_Direct3D->ResizeBuffers(screenWidth, screenHeight);
}

POINT Application::MousePosition()
{
    POINT mousePosition = { m_mouseX, m_mouseY };

    // Poll DirectInput for the latest mouse movement and fold it into the tracked position.
    if (ReadMouse())
    {
        m_mouseX += m_mouseState.lX;
        m_mouseY += m_mouseState.lY;

        // Keep the position clamped inside the window bounds.
        if (m_mouseX < 0) { m_mouseX = 0; }
        if (m_mouseY < 0) { m_mouseY = 0; }
        if (m_mouseX > m_screenWidth)  { m_mouseX = m_screenWidth; }
        if (m_mouseY > m_screenHeight) { m_mouseY = m_screenHeight; }

        mousePosition.x = m_mouseX;
        mousePosition.y = m_mouseY;
    }

    return mousePosition;
}


bool Application::ReadMouse()
{
    HRESULT result;

    // Read the mouse device.
    result = m_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&m_mouseState);
    if (FAILED(result))
    {
        // If the mouse lost focus or was not acquired then try to get control back.
        if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
        {
            m_mouse->Acquire();
        }
        return false;
    }

    return true;
}


bool Application::Render()
{
    
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix;
    bool result;


    // Clear the buffers to begin the scene.
    m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    // Generate the view matrix based on the camera's position.
    m_Camera->Render();

    // Get the world, view, and projection matrices from the camera and d3d objects.
    m_Direct3D->GetWorldMatrix(worldMatrix);
    m_Camera->GetViewMatrix(viewMatrix);
    m_Direct3D->GetProjectionMatrix(projectionMatrix);

    // Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
    m_Model->Render(m_Direct3D->GetDeviceContext());

    // NOTE: the color shader no longer draws this model. ModelClass::VertexType now carries a
    // TEXCOORD instead of a COLOR, so the color shader's input layout does not match the vertex
    // buffer any more. It also wrote depth first, which made the textured pass below fail the
    // D3D11_COMPARISON_LESS depth test.

    // Render the model using the texture shader.
    result = m_TextureShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture());
    if (!result)
    {
        return false;
    }
    
    // Present the rendered scene to the screen.
    m_Direct3D->EndScene();

    return true;
}

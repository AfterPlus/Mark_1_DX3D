#include "Application.h"
#include "InputClass.h"

Application::Application()
{
    m_Direct3D = nullptr;
    m_directInput = nullptr;
    m_mouse = nullptr;
    m_mousePosition = { 0, 0 };
    m_Camera = nullptr;
    m_Model = nullptr;
    m_ColorShader = nullptr;
    m_TextureShader = nullptr ;
    m_Light = nullptr;
    m_LightShader = nullptr;
    m_Light = nullptr;
    m_Sprite = nullptr;
    m_Timer = nullptr;
}

Application::Application(const Application& other)
{
}

Application::~Application()
{
}

bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    char spriteFilename[128];
    char modelFilename[128];
    char textureFilename[128];
    bool result;

    // Create and initialize new direct X object
    m_Direct3D = new D3dClass;
    
    // Set the model and texture filenames.
    strcpy_s(modelFilename, "_Shader/plane.txt");
    strcpy_s(textureFilename, "_Shader/stone01.tga");

    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
        return false;
    }
    
    // Create the camera object.
    m_Camera = new CameraClass;

    // Set the initial position of the camera.
    m_Camera->SetPosition(0.0f, 0.0f, -10.0f);
    m_Camera->Render();

    // Create and initialize the model object.
    m_Model = new ModelClass;

    result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), textureFilename, modelFilename);
    
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

    // Create and initialize the light shader object.
    m_LightShader = new LightShaderClass;

    result = m_LightShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
        return false;
    }
    
    // Set the file name of the sprite data file.
    strcpy_s(spriteFilename, "_Shader/sprite_data_01.txt");

    // Create and initialize the sprite object.
    m_Sprite = new SpriteClass;

    result = m_Sprite->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, spriteFilename, 50, 50);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the sprite object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the timer object.
    m_Timer = new TimerClass;

    result = m_Timer->Initialize();
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the timer object.", L"Error", MB_OK);
        return false;
    }
    
    // Set the number of lights we will use.
    m_numLights = 4;

    // Create and initialize the light objects array.
    m_Lights = new LightClass[m_numLights];

    // Manually set the color and position of each light.
    m_Lights[0].SetDiffuseColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    m_Lights[0].SetPosition(-3.0f, 1.0f, 3.0f);

    m_Lights[1].SetDiffuseColor(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    m_Lights[1].SetPosition(3.0f, 1.0f, 3.0f);

    m_Lights[2].SetDiffuseColor(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
    m_Lights[2].SetPosition(-3.0f, 1.0f, -3.0f);

    m_Lights[3].SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);  // White
    m_Lights[3].SetPosition(3.0f, 1.0f, -3.0f);
    
    return true;
    
}

void Application::Shutdown()
{
    // Release the timer object.
    if(m_Timer)
    {
        delete m_Timer;
        m_Timer = nullptr;
    }

    // Release the sprite object.
    if(m_Sprite)
    {
        m_Sprite->Shutdown();
        delete m_Sprite;
        m_Sprite = nullptr;
    }
    
    // Release the light objects.
    if(m_Lights)
    {
        delete [] m_Lights;
        m_Lights = nullptr;
    }
    
    // Release the light object.
    if(m_Light)
    {
        delete m_Light;
        m_Light = nullptr;
    }

    // Release the light shader object.
    if(m_LightShader)
    {
        m_LightShader->Shutdown();
        delete m_LightShader;
        m_LightShader = nullptr;
    }
    
    // Release the texture shader object.
    if (m_TextureShader)
    {
        m_TextureShader->Shutdown();
        delete m_TextureShader;
        m_TextureShader = nullptr;
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
        m_ColorShader = nullptr;
    }

    // Release the model object.
    if (m_Model)
    {
        m_Model->Shutdown();
        delete m_Model;
        m_Model = nullptr;
    }

    // Release the camera object.
    if (m_Camera)
    {
        delete m_Camera;
        m_Camera = nullptr;
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
    static float rotation = 0.0f;
    float frameTime;
    bool result;

    // Update the system stats.
    m_Timer->Frame();

    // Get the current frame time.
    frameTime = m_Timer->GetTime();

    // Update the sprite object using the frame time.
    m_Sprite->Update(frameTime);
    
    // Update the rotation variable each frame.
    rotation -= 0.0174532925f * 0.9f;
    if(rotation < 0.0f)
    {
        rotation += 360.0f;
    }
    
    // Render the graphics scene.
    result = Render(rotation);
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

    if (!m_Direct3D->ResizeBuffers(screenWidth, screenHeight))
    {
        return false;
    }

    // The sprite quad is positioned in pixels relative to the screen centre, so it needs the new size.
    if (m_Sprite)
    {
        m_Sprite->SetScreenSize(screenWidth, screenHeight);
    }

    return true;
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


bool Application::Render(float rotation)
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;
    XMFLOAT4 diffuseColor[4], lightPosition[4];
    int i;
    bool result;


    // Clear the buffers to begin the scene.
    m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    // Generate the view matrix based on the camera's position.
    m_Camera->Render();

    // Get the world, view, and projection matrices from the camera and d3d objects.
    m_Direct3D->GetWorldMatrix(worldMatrix);
    m_Camera->GetViewMatrix(viewMatrix);
    //m_Direct3D->GetProjectionMatrix(projectionMatrix);
    m_Direct3D->GetOrthoMatrix(orthoMatrix);

    // Turn off the Z buffer to begin all 2D rendering.
    m_Direct3D->TurnZBufferOff();
    
    // Put the sprite vertex and index buffers on the graphics pipeline to prepare them for drawing.
    result = m_Sprite->Render(m_Direct3D->GetDeviceContext());
    if(!result)
    {
        return false;
    }
    
    // Render the sprite with the texture shader.
    result = m_TextureShader->Render(m_Direct3D->GetDeviceContext(), m_Sprite->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix, m_Sprite->GetTexture());
    if(!result)
    {
        return false;
    }
    
    // Turn the Z buffer back on now that all 2D rendering has completed.
    m_Direct3D->TurnZBufferOn();

    // Present the rendered scene to the screen.
    m_Direct3D->EndScene();

    return true;
}

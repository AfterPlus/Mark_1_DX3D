#include "Application.h"

Application::Application()
{
    m_Direct3D = nullptr;
    m_Camera = nullptr;
    m_Model = nullptr;
    m_ColorShader = nullptr;
    m_TextureShader = nullptr ;
    m_LightShader = nullptr;
    m_Light = nullptr;
    m_Lights = nullptr;
    m_numLights = 0;
    m_screenWidth = 0;
    m_screenHeight = 0;
    m_Sprite = nullptr;
    m_Timer = nullptr;
    m_FontShader = nullptr;
    m_Font = nullptr;
    m_Fps = nullptr;
    m_FpsString = nullptr;
    m_TextString1 = nullptr;
    m_TextString2 = nullptr;
    m_MouseStrings = nullptr;
    m_previousFps = -1;
    m_MultiTextureShader = nullptr;
}

Application::Application(const Application& other)
{
}

Application::~Application()
{
}

bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    char textureFilename1[128];
    char textureFilename2[128];
    char spriteFilename[128];
    char modelFilename[128];
    char fpsString[32];
    char mouseString1[32], mouseString2[32], mouseString3[32];
    char helloString[32], goodbyeString[32];
    bool result;

    // Store the screen size so the text can be repositioned if the window is resized.
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Create and initialize new direct X object
    m_Direct3D = new D3dClass;

    // Set the model and texture filenames.
    strcpy_s(modelFilename, "_Shader/plane.txt");
    strcpy_s(textureFilename1, "_Shader/stone01.tga");
    strcpy_s(textureFilename2, "_Shader/dirt01.tga");

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
    m_Camera->Render();

    // Create and initialize the multitexture shader object.
    m_MultiTextureShader = new MultiTextureShaderClass;

    result = m_MultiTextureShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the multitexture shader object.", L"Error", MB_OK);
        return false;
    }
    
    // Create and initialize the model object.
    m_Model = new ModelClass;

    result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, textureFilename1, textureFilename2);

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

    result = m_Sprite->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, spriteFilename, 500, 200);
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

    // Create and initialize the font shader object.
    m_FontShader = new FontShaderClass;

    result = m_FontShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the font shader object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the font object.
    m_Font = new FontClass;

    result = m_Font->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), 0);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the font object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the fps object.
    m_Fps = new FpsClass;

    m_Fps->Initialize();

    // Set the initial fps and fps string.
    m_previousFps = -1;
    strcpy_s(fpsString, "Fps: 0");

    // Create and initialize the text object for the fps string.
    m_FpsString = new TextClass;

    result = m_FpsString->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, fpsString, 10, 10, 0.0f, 1.0f, 0.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the fps text object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the text objects for the mouse strings.
    strcpy_s(mouseString1, "Mouse X: 0");
    strcpy_s(mouseString2, "Mouse Y: 0");
    strcpy_s(mouseString3, "Mouse Button: No");

    m_MouseStrings = new TextClass[3];

    result = m_MouseStrings[0].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, mouseString1, 10, 40, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the mouse text objects.", L"Error", MB_OK);
        return false;
    }

    result = m_MouseStrings[1].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, mouseString2, 10, 70, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the mouse text objects.", L"Error", MB_OK);
        return false;
    }

    result = m_MouseStrings[2].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, mouseString3, 10, 100, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the mouse text objects.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the first text object.
    strcpy_s(helloString, "Hello");

    m_TextString1 = new TextClass;

    result = m_TextString1->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, helloString, 100, 200, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the text object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the second text object.
    strcpy_s(goodbyeString, "Goodbye");

    m_TextString2 = new TextClass;

    result = m_TextString2->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, goodbyeString, 100, 250, 1.0f, 1.0f, 0.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the text object.", L"Error", MB_OK);
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
    // Release the multitexture shader object.
    if(m_MultiTextureShader)
    {
        m_MultiTextureShader->Shutdown();
        delete m_MultiTextureShader;
        m_MultiTextureShader = nullptr;
    }
    
    // Release the text objects.
    if(m_TextString2)
    {
        m_TextString2->Shutdown();
        delete m_TextString2;
        m_TextString2 = nullptr;
    }

    if(m_TextString1)
    {
        m_TextString1->Shutdown();
        delete m_TextString1;
        m_TextString1 = nullptr;
    }

    // Release the mouse string text objects.
    if(m_MouseStrings)
    {
        m_MouseStrings[0].Shutdown();
        m_MouseStrings[1].Shutdown();
        m_MouseStrings[2].Shutdown();

        delete [] m_MouseStrings;
        m_MouseStrings = nullptr;
    }

    // Release the text object for the fps string.
    if(m_FpsString)
    {
        m_FpsString->Shutdown();
        delete m_FpsString;
        m_FpsString = nullptr;
    }

    // Release the fps object.
    if(m_Fps)
    {
        delete m_Fps;
        m_Fps = nullptr;
    }

    // Release the font object.
    if(m_Font)
    {
        m_Font->Shutdown();
        delete m_Font;
        m_Font = nullptr;
    }

    // Release the font shader object.
    if(m_FontShader)
    {
        m_FontShader->Shutdown();
        delete m_FontShader;
        m_FontShader = nullptr;
    }

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


bool Application::Frame(InputClass* Input)
{
    static float rotation = 0.0f;
    float frameTime;
    int mouseX, mouseY;
    bool result, mouseDown;

    // Check if the user pressed escape and wants to exit the application.
    if(Input->IsEscapePressed())
    {
        return false;
    }

    // Get the location of the mouse from the input object.
    Input->GetMouseLocation(mouseX, mouseY);

    // Check if the mouse has been pressed.
    mouseDown = Input->IsMousePressed();

    // Update the mouse strings each frame.
    result = UpdateMouseStrings(mouseX, mouseY, mouseDown);
    if(!result)
    {
        return false;
    }

    // Update the frames per second each frame.
    result = UpdateFps();
    if(!result)
    {
        return false;
    }

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
    char helloString[32], goodbyeString[32];

    if (!m_Direct3D)
    {
        return false;
    }

    if (!m_Direct3D->ResizeBuffers(screenWidth, screenHeight))
    {
        return false;
    }

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // The sprite quad is positioned in pixels relative to the screen centre, so it needs the new size.
    if (m_Sprite)
    {
        m_Sprite->SetScreenSize(screenWidth, screenHeight);
    }

    // The text quads are positioned the same way. The fps and mouse strings get rebuilt by their per-frame
    // updates, but the static strings have to be rebuilt here.
    if (m_FpsString)
    {
        m_FpsString->SetScreenSize(screenWidth, screenHeight);
    }

    if (m_MouseStrings)
    {
        m_MouseStrings[0].SetScreenSize(screenWidth, screenHeight);
        m_MouseStrings[1].SetScreenSize(screenWidth, screenHeight);
        m_MouseStrings[2].SetScreenSize(screenWidth, screenHeight);
    }

    if (m_TextString1)
    {
        strcpy_s(helloString, "Hello");
        m_TextString1->SetScreenSize(screenWidth, screenHeight);
        m_TextString1->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, helloString, 100, 200, 1.0f, 1.0f, 1.0f);
    }

    if (m_TextString2)
    {
        strcpy_s(goodbyeString, "Goodbye");
        m_TextString2->SetScreenSize(screenWidth, screenHeight);
        m_TextString2->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, goodbyeString, 100, 250, 1.0f, 1.0f, 0.0f);
    }

    // Force the fps string to be rebuilt against the new screen size on the next frame.
    m_previousFps = -1;

    return true;
}

bool Application::Render(float rotation)
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix;
    bool result;


    // Clear the buffers to begin the scene.
    m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    // Get the world, view, and projection matrices from the camera and d3d objects.
    m_Direct3D->GetWorldMatrix(worldMatrix);
    m_Camera->GetViewMatrix(viewMatrix);
    m_Direct3D->GetProjectionMatrix(projectionMatrix);

    // Render the model using the multitexture shader.
    m_Model->Render(m_Direct3D->GetDeviceContext());

    result = m_MultiTextureShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, 
                                          m_Model->GetTexture(0), m_Model->GetTexture(1));
    if(!result)
    {
        return false;
    }

    // Present the rendered scene to the screen.
    m_Direct3D->EndScene();

    return true;
}

bool Application::UpdateFps()
{
    int fps;
    char tempString[16], finalString[16];
    float red, green, blue;
    bool result;


    // Update the fps each frame.
    m_Fps->Frame();

    // Get the current fps.
    fps = m_Fps->GetFps();

    // Check if the fps from the previous frame was the same, if so don't need to update the text string.
    if(m_previousFps == fps)
    {
        return true;
    }

    // Store the fps for checking next frame.
    m_previousFps = fps;

    // Truncate the fps to below 100,000.
    if(fps > 99999)
    {
        fps = 99999;
    }

    // Convert the fps integer to string format.
    sprintf_s(tempString, "%d", fps);

    // Setup the fps string.
    strcpy_s(finalString, "Fps: ");
    strcat_s(finalString, tempString);

    // If fps is 60 or above set the fps color to green.
    if(fps >= 60)
    {
        red = 0.0f;
        green = 1.0f;
        blue = 0.0f;
    }

    // If fps is below 60 set the fps color to yellow.
    if(fps < 60)
    {
        red = 1.0f;
        green = 1.0f;
        blue = 0.0f;
    }

    // If fps is below 30 set the fps color to red.
    if(fps < 30)
    {
        red = 1.0f;
        green = 0.0f;
        blue = 0.0f;
    }

    // Update the sentence vertex buffer with the new string information.
    result = m_FpsString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 10, red, green, blue);
    if(!result)
    {
        return false;
    }

    return true;
}

bool Application::UpdateMouseStrings(int mouseX, int mouseY, bool mouseDown)
{
    char tempString[16], finalString[32];
    bool result;


    // Convert the mouse X integer to string format.
    sprintf_s(tempString, "%d", mouseX);

    // Setup the mouse X string.
    strcpy_s(finalString, "Mouse X: ");
    strcat_s(finalString, tempString);

    // Update the sentence vertex buffer with the new string information.
    result = m_MouseStrings[0].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 40, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        return false;
    }

    // Convert the mouse Y integer to string format.
    sprintf_s(tempString, "%d", mouseY);

    // Setup the mouse Y string.
    strcpy_s(finalString, "Mouse Y: ");
    strcat_s(finalString, tempString);

    // Update the sentence vertex buffer with the new string information.
    result = m_MouseStrings[1].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 70, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        return false;
    }

    // Setup the mouse button string.
    if(mouseDown)
    {
        strcpy_s(finalString, "Mouse Button: Yes");
    }
    else
    {
        strcpy_s(finalString, "Mouse Button: No");
    }

    // Update the sentence vertex buffer with the new string information.
    result = m_MouseStrings[2].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 100, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        return false;
    }

    return true;
}

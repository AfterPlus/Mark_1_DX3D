#include "Application.h"

// Layout of the HUD widgets, in pixels.
static const int WIDGET_MARGIN = 10;     // Gap between an expanded widget and the screen edge.
static const int WIDGET_LINE_HEIGHT = 30;
static const int WIDGET_HEADER_HEIGHT = 34;
static const int WIDGET_TITLE_GAP = 8;   // Gap between the minimize button and the widget title.
static const int WIDGET_BUTTON_PAD = 4;  // Extra clickable pixels around the minimize button.

Application::Application()
{
    m_Direct3D = nullptr;
    m_Camera = nullptr;
    m_Model = nullptr;
    m_ColorShader = nullptr;
    m_TextureShader = nullptr ;
    m_LightShader = nullptr;
    m_Light = nullptr;
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
    m_LightMapShader = nullptr;
    m_AlphaMapShader = nullptr;
    m_NormalMapShader = nullptr;
    m_AlphaModel = nullptr;
    m_NormalModel = nullptr;
    m_RenderCountString = nullptr;
    m_ModelList = nullptr;
    m_Position = nullptr;
    m_Frustum = nullptr;
    m_RenderTexture = nullptr;
    m_DisplayPlane = nullptr;
    m_cubeRotation = 0.0f;
    m_mouseWasDown = false;
    m_widgetsInitialized = false;
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
    char textureFilename3[128];
    char spriteFilename[128];
    char modelFilename[128];
    char fpsString[32];
    char mouseString1[32], mouseString2[32], mouseString3[32];
    char helloString[32], goodbyeString[32];
    char renderCountString[32];
    XMMATRIX baseViewMatrix;
    bool result;

    // Store the screen size so the text can be repositioned if the window is resized.
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Create and initialize new direct X object
    m_Direct3D = new D3dClass;

    // Set the model and texture filenames.
    strcpy_s(modelFilename, "_Shader/Sphere.txt");
    strcpy_s(textureFilename1, "_Shader/stone01.tga");
    strcpy_s(textureFilename2, "_Shader/light01.tga");

    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
        return false;
    }

    // Create the camera object.
    m_Camera = new CameraClass;

    // Store a base view matrix for the 2D text. The camera has to be in front of the ortho near plane (z = 0),
    // so render it once at z = -10 and keep the unrotated matrix; the text then stays fixed on screen.
    m_Camera->SetPosition(0.0f, 0.0f, -10.0f);
    m_Camera->Render();
    m_Camera->GetViewMatrix(baseViewMatrix);
    XMStoreFloat4x4(&m_baseViewMatrix, baseViewMatrix);

    // Set the initial position of the camera for the 3D scene.
    m_Camera->SetPosition(0.0f, 0.0f, 0.0f);
    m_Camera->Render();
    
    // Create and initialize the light map shader object.
    m_LightMapShader = new LightMapShaderClass;

    result = m_LightMapShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the light map shader object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the alpha map shader object.
    m_AlphaMapShader = new AlphaMapShaderClass;

    result = m_AlphaMapShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the alpha map shader object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the normal map shader object.
    m_NormalMapShader = new NormalMapShaderClass;

    result = m_NormalMapShader->Initialize(m_Direct3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the normal map shader object.", L"Error", MB_OK);
        return false;
    }

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

    // Create and initialize the alpha map model (square blending stone and dirt through the alpha map).
    strcpy_s(modelFilename, "_Shader/square.txt");
    strcpy_s(textureFilename1, "_Shader/stone01.tga");
    strcpy_s(textureFilename2, "_Shader/dirt01.tga");
    strcpy_s(textureFilename3, "_Shader/alpha01.tga");

    m_AlphaModel = new ModelClass;

    result = m_AlphaModel->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, textureFilename1, textureFilename2, textureFilename3);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the alpha map model object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the normal map model (cube with a color texture and a normal map).
    strcpy_s(modelFilename, "_Shader/Cube.txt");
    strcpy_s(textureFilename1, "_Shader/stone01.tga");
    strcpy_s(textureFilename2, "_Shader/normal01.tga");

    m_NormalModel = new ModelClass;

    result = m_NormalModel->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, textureFilename1, textureFilename2);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the normal map model object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the light object used by the normal map shader.
    m_Light = new LightClass;

    m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_Light->SetDirection(0.0f, 0.0f, 1.0f);

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

    // Create and initialize the render count text object.
    strcpy_s(renderCountString, "Render Count: 0");

    m_RenderCountString = new TextClass;

    result = m_RenderCountString->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, renderCountString, 10, 130, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the render count text object.", L"Error", MB_OK);
        return false;
    }

    // Group the text into the minimizable HUD widgets and place them.
    result = InitializeWidgets();
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the HUD widgets.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the model list object with 25 randomly positioned spheres.
    m_ModelList = new ModelListClass;

    m_ModelList->Initialize(25);

    // Create the position object used to rotate the camera.
    m_Position = new PositionClass;

    // Create the frustum object.
    m_Frustum = new FrustumClass;

    // Create and initialize the render texture the spinning cube is drawn into (256x256, 32 bit float format).
    m_RenderTexture = new RenderTextureClass;

    result = m_RenderTexture->Initialize(m_Direct3D->GetDevice(), 256, 256, SCREEN_DEPTH, SCREEN_NEAR, 1);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the render texture object.", L"Error", MB_OK);
        return false;
    }

    // Create and initialize the display plane that shows the render texture. 1x1 matches the square render texture.
    m_DisplayPlane = new DisplayPlaneClass;

    result = m_DisplayPlane->Initialize(m_Direct3D->GetDevice(), 1.0f, 1.0f);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the display plane object.", L"Error", MB_OK);
        return false;
    }

    return true;

}

void Application::Shutdown()
{
    // Release the HUD widget buttons and titles.
    ShutdownWidgets();

    // Release the display plane and render texture objects.
    if(m_DisplayPlane)
    {
        m_DisplayPlane->Shutdown();
        delete m_DisplayPlane;
        m_DisplayPlane = nullptr;
    }

    if(m_RenderTexture)
    {
        m_RenderTexture->Shutdown();
        delete m_RenderTexture;
        m_RenderTexture = nullptr;
    }

    // Release the frustum, position and model list objects.
    if(m_Frustum)
    {
        delete m_Frustum;
        m_Frustum = nullptr;
    }

    if(m_Position)
    {
        delete m_Position;
        m_Position = nullptr;
    }

    if(m_ModelList)
    {
        m_ModelList->Shutdown();
        delete m_ModelList;
        m_ModelList = nullptr;
    }

    // Release the render count text object.
    if(m_RenderCountString)
    {
        m_RenderCountString->Shutdown();
        delete m_RenderCountString;
        m_RenderCountString = nullptr;
    }

    // Release the normal map and alpha map objects.
    if(m_NormalModel)
    {
        m_NormalModel->Shutdown();
        delete m_NormalModel;
        m_NormalModel = nullptr;
    }

    if(m_AlphaModel)
    {
        m_AlphaModel->Shutdown();
        delete m_AlphaModel;
        m_AlphaModel = nullptr;
    }

    if(m_NormalMapShader)
    {
        m_NormalMapShader->Shutdown();
        delete m_NormalMapShader;
        m_NormalMapShader = nullptr;
    }

    if(m_AlphaMapShader)
    {
        m_AlphaMapShader->Shutdown();
        delete m_AlphaMapShader;
        m_AlphaMapShader = nullptr;
    }

    // Release the multitexture shader object.
    if(m_MultiTextureShader)
    {
        m_MultiTextureShader->Shutdown();
        delete m_MultiTextureShader;
        m_MultiTextureShader = nullptr;
    }
    
    if(m_LightMapShader)
    {
        m_LightMapShader->Shutdown();
        delete m_LightMapShader;
        m_LightMapShader = 0;
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
    float frameTime, rotationY;
    int mouseX, mouseY;
    bool result, mouseDown, keyDown;

    // Check if the user pressed escape and wants to exit the application.
    if(Input->IsEscapePressed())
    {
        return false;
    }

    // Get the location of the mouse from the input object.
    Input->GetMouseLocation(mouseX, mouseY);

    // Check if the mouse has been pressed.
    mouseDown = Input->IsMousePressed();

    // A new left click on a widget's minimize button toggles that widget. Only the press itself counts, not holding the button.
    if(mouseDown && !m_mouseWasDown && Input->IsMouseInWindow())
    {
        result = HandleWidgetClick(mouseX, mouseY);
        if(!result)
        {
            return false;
        }
    }

    m_mouseWasDown = mouseDown;

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

    // The position object's turn speeds are tuned for the frame time in milliseconds, the timer reports seconds.
    m_Position->SetFrameTime(frameTime * 1000.0f);

    // Check if the left or right arrow key has been pressed, if so rotate the camera accordingly.
    keyDown = Input->IsLeftArrowPressed();
    m_Position->TurnLeft(keyDown);

    keyDown = Input->IsRightArrowPressed();
    m_Position->TurnRight(keyDown);

    // Get the current view point rotation.
    m_Position->GetRotation(rotationY);

    // Set the rotation of the camera.
    m_Camera->SetRotation(0.0f, rotationY, 0.0f);

    // Spin the cube that is drawn into the render texture, keeping the angle in [0, 2*pi).
    m_cubeRotation += frameTime * 1.0f;
    if(m_cubeRotation > XM_2PI)
    {
        m_cubeRotation -= XM_2PI;
    }

    // Render the cube to the render texture first, the main scene then displays that texture.
    result = RenderSceneToTexture();
    if(!result)
    {
        return false;
    }

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
    int i;

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

    if (m_RenderCountString)
    {
        m_RenderCountString->SetScreenSize(screenWidth, screenHeight);
    }

    if (m_TextString1)
    {
        m_TextString1->SetScreenSize(screenWidth, screenHeight);
    }

    if (m_TextString2)
    {
        m_TextString2->SetScreenSize(screenWidth, screenHeight);
    }

    // The widget titles and buttons are text quads too.
    for (i = 0; i < WIDGET_COUNT; i++)
    {
        m_Widgets[i].title.SetScreenSize(screenWidth, screenHeight);
        m_Widgets[i].button.SetScreenSize(screenWidth, screenHeight);
    }

    // The widgets are docked to the screen edges, so place them again and rebuild the static strings.
    return LayoutWidgets();
}

bool Application::RenderSceneToTexture()
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix;
    bool result;

    // Redirect the output to the render texture and clear it to a light blue.
    m_RenderTexture->SetRenderTarget(m_Direct3D->GetDeviceContext());
    m_RenderTexture->ClearRenderTarget(m_Direct3D->GetDeviceContext(), 0.0f, 0.5f, 1.0f, 1.0f);

    // Use a fixed view of the cube so the main camera is left untouched.
    viewMatrix = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    // The render texture has its own projection matrix, the screen one would stretch the image.
    m_RenderTexture->GetProjectionMatrix(projectionMatrix);

    worldMatrix = XMMatrixRotationY(m_cubeRotation);

    m_NormalModel->Render(m_Direct3D->GetDeviceContext());

    result = m_TextureShader->Render(m_Direct3D->GetDeviceContext(), m_NormalModel->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
                                     m_NormalModel->GetTexture(0));
    if(!result)
    {
        return false;
    }

    // Go back to rendering to the back buffer and its viewport.
    m_Direct3D->SetBackBufferRenderTarget();
    m_Direct3D->ResetViewport();

    return true;
}

bool Application::Render()
{
    XMMATRIX worldMatrix, viewMatrix, baseViewMatrix, projectionMatrix, orthoMatrix;
    float positionX, positionY, positionZ, radius;
    int modelCount, renderCount, i;
    bool renderModel, result;


    // Clear the buffers to begin the scene.
    m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    // Generate the view matrix based on the camera's position and rotation.
    m_Camera->Render();

    // Get the world, view, projection and ortho matrices from the camera and d3d objects.
    m_Direct3D->GetWorldMatrix(worldMatrix);
    m_Camera->GetViewMatrix(viewMatrix);
    m_Direct3D->GetProjectionMatrix(projectionMatrix);
    m_Direct3D->GetOrthoMatrix(orthoMatrix);

    // Construct the frustum for this frame.
    m_Frustum->ConstructFrustum(viewMatrix, projectionMatrix, SCREEN_DEPTH);

    // Get the number of models that will be rendered.
    modelCount = m_ModelList->GetModelCount();

    // Initialize the count of models that have been rendered.
    renderCount = 0;

    // Go through all the models and render them only if they can be seen by the camera view.
    for(i=0; i<modelCount; i++)
    {
        // Get the position of the sphere model at this index.
        m_ModelList->GetData(i, positionX, positionY, positionZ);

        // Set the radius of the sphere to 1.0 since this is already known.
        radius = 1.0f;

        // Check if the sphere model is in the view frustum.
        renderModel = m_Frustum->CheckSphere(positionX, positionY, positionZ, radius);

        // If it can be seen then render it, if not skip this model and check the next sphere.
        if(renderModel)
        {
            // Move the model to the location it should be rendered at.
            worldMatrix = XMMatrixTranslation(positionX, positionY, positionZ);

            // Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
            m_Model->Render(m_Direct3D->GetDeviceContext());

            // Render the model using the texture shader, which applies full ambient light (the plain texture color).
            result = m_TextureShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
                                             m_Model->GetTexture(0));
            if(!result)
            {
                return false;
            }

            // Since this model was rendered then increase the count for this frame.
            renderCount++;
        }
    }

    // Draw three planes showing the render texture, placed in the world in front of the camera.
    static const float planeX[3] = { 0.0f, -1.5f, 1.5f };
    static const float planeY[3] = { 1.5f, -1.5f, -1.5f };

    for(i=0; i<3; i++)
    {
        worldMatrix = XMMatrixTranslation(planeX[i], planeY[i], 8.0f);

        m_DisplayPlane->Render(m_Direct3D->GetDeviceContext());

        result = m_TextureShader->Render(m_Direct3D->GetDeviceContext(), m_DisplayPlane->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
                                         m_RenderTexture->GetShaderResourceView());
        if(!result)
        {
            return false;
        }
    }

    // Update the render count text.
    result = UpdateRenderCountString(renderCount);
    if(!result)
    {
        return false;
    }

    // Reset the world matrix and use the unrotated base view for the 2D rendering so the text stays on screen.
    m_Direct3D->GetWorldMatrix(worldMatrix);
    baseViewMatrix = XMLoadFloat4x4(&m_baseViewMatrix);

    // Turn off the Z buffer and turn on alpha blending to begin all 2D rendering.
    m_Direct3D->TurnZBufferOff();
    m_Direct3D->EnableAlphaBlending();

    // Render the HUD widgets (titles, minimize buttons and, for expanded widgets, their text lines).
    result = RenderWidgets(worldMatrix, baseViewMatrix, orthoMatrix);
    if(!result)
    {
        return false;
    }

    // Turn the Z buffer back on and disable alpha blending now that all 2D rendering has completed.
    m_Direct3D->TurnZBufferOn();
    m_Direct3D->DisableAlphaBlending();

    // Present the rendered scene to the screen.
    m_Direct3D->EndScene();

    return true;
}

bool Application::UpdateRenderCountString(int renderCount)
{
    char tempString[16], finalString[32];
    int textX, textY;

    // Convert the render count integer to string format.
    sprintf_s(tempString, "%d", renderCount);

    // Setup the render count string.
    strcpy_s(finalString, "Render Count: ");
    strcat_s(finalString, tempString);

    // Update the sentence vertex buffer with the new string information.
    GetLinePosition(WIDGET_RENDER, 0, textX, textY);

    return m_RenderCountString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, textX, textY, 1.0f, 1.0f, 1.0f);
}

bool Application::UpdateFps()
{
    int fps;
    char tempString[16], finalString[16];
    float red, green, blue;
    int textX, textY;
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
    GetLinePosition(WIDGET_FPS, 0, textX, textY);

    result = m_FpsString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, textX, textY, red, green, blue);
    if(!result)
    {
        return false;
    }

    return true;
}

bool Application::UpdateMouseStrings(int mouseX, int mouseY, bool mouseDown)
{
    char tempString[16], finalString[32];
    int textX, textY;
    bool result;


    // Convert the mouse X integer to string format.
    sprintf_s(tempString, "%d", mouseX);

    // Setup the mouse X string.
    strcpy_s(finalString, "Mouse X: ");
    strcat_s(finalString, tempString);

    // Update the sentence vertex buffer with the new string information.
    GetLinePosition(WIDGET_MOUSE, 0, textX, textY);
    result = m_MouseStrings[0].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, textX, textY, 1.0f, 1.0f, 1.0f);
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
    GetLinePosition(WIDGET_MOUSE, 1, textX, textY);
    result = m_MouseStrings[1].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, textX, textY, 1.0f, 1.0f, 1.0f);
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
    GetLinePosition(WIDGET_MOUSE, 2, textX, textY);
    result = m_MouseStrings[2].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, textX, textY, 1.0f, 1.0f, 1.0f);
    if(!result)
    {
        return false;
    }

    return true;
}

// The arrow on a widget's button points toward the screen edge the widget is docked to while it is expanded (it will
// collapse that way), and back toward the screen once it is minimized (it will expand that way).
static const char* GetButtonText(DockEdge edge, bool minimized)
{
    switch(edge)
    {
        case DOCK_LEFT:   { return minimized ? "[>]" : "[<]"; }
        case DOCK_RIGHT:  { return minimized ? "[<]" : "[>]"; }
        case DOCK_TOP:    { return minimized ? "[v]" : "[^]"; }
        default:          { return minimized ? "[^]" : "[v]"; }
    }
}

bool Application::InitializeWidgets()
{
    // The widest line each widget can show, used to size the widget so it does not change width as the numbers change.
    static const char* widestLines[WIDGET_COUNT] = { "Fps: 99999", "Render Count: 999", "Mouse Button: Yes", "Goodbye" };
    HudWidget* widget;
    char text[32];
    int i, lineWidth, headerWidth;
    bool result;

    // Fps readout on the left edge.
    widget = &m_Widgets[WIDGET_FPS];
    widget->titleText = "Fps";
    widget->edge = DOCK_LEFT;
    widget->anchor = WIDGET_MARGIN;
    widget->lineCount = 1;
    widget->lines[0] = m_FpsString;

    // Render count readout on the top edge.
    widget = &m_Widgets[WIDGET_RENDER];
    widget->titleText = "Culling";
    widget->edge = DOCK_TOP;
    widget->anchor = 200;
    widget->lineCount = 1;
    widget->lines[0] = m_RenderCountString;

    // Mouse readout on the right edge.
    widget = &m_Widgets[WIDGET_MOUSE];
    widget->titleText = "Mouse";
    widget->edge = DOCK_RIGHT;
    widget->anchor = WIDGET_MARGIN;
    widget->lineCount = 3;
    widget->lines[0] = &m_MouseStrings[0];
    widget->lines[1] = &m_MouseStrings[1];
    widget->lines[2] = &m_MouseStrings[2];

    // Greetings on the bottom edge.
    widget = &m_Widgets[WIDGET_MESSAGES];
    widget->titleText = "Messages";
    widget->edge = DOCK_BOTTOM;
    widget->anchor = WIDGET_MARGIN;
    widget->lineCount = 2;
    widget->lines[0] = m_TextString1;
    widget->lines[1] = m_TextString2;

    for(i=0; i<WIDGET_COUNT; i++)
    {
        widget = &m_Widgets[i];

        widget->minimized = false;
        widget->buttonX = 0;
        widget->buttonY = 0;
        widget->lineX = 0;
        widget->lineY = 0;

        // Create the title text, the layout moves it into place.
        strcpy_s(text, widget->titleText);

        result = widget->title.Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), m_screenWidth, m_screenHeight, 32, m_Font, text, 0, 0, 1.0f, 1.0f, 1.0f);
        if(!result)
        {
            return false;
        }

        // Create the minimize button text.
        strcpy_s(text, GetButtonText(widget->edge, false));
        widget->buttonWidth = m_Font->GetSentencePixelLength(text);

        result = widget->button.Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), m_screenWidth, m_screenHeight, 8, m_Font, text, 0, 0, 0.4f, 0.8f, 1.0f);
        if(!result)
        {
            return false;
        }

        // The widget is as wide as its widest line or its header, whichever is bigger.
        strcpy_s(text, widestLines[i]);
        lineWidth = m_Font->GetSentencePixelLength(text);

        strcpy_s(text, widget->titleText);
        headerWidth = widget->buttonWidth + WIDGET_TITLE_GAP + m_Font->GetSentencePixelLength(text);

        widget->width = (lineWidth > headerWidth) ? lineWidth : headerWidth;
    }

    m_widgetsInitialized = true;

    return LayoutWidgets();
}

void Application::ShutdownWidgets()
{
    int i;

    for(i=0; i<WIDGET_COUNT; i++)
    {
        m_Widgets[i].button.Shutdown();
        m_Widgets[i].title.Shutdown();
    }

    m_widgetsInitialized = false;
}

bool Application::LayoutWidgets()
{
    HudWidget* widget;
    ID3D11DeviceContext* deviceContext;
    XMFLOAT4 color;
    char text[32];
    int i, edgeInset, panelX, panelY, titleX, expandedHeight, textX, textY;
    bool result;

    if(!m_widgetsInitialized)
    {
        return true;
    }

    deviceContext = m_Direct3D->GetDeviceContext();

    for(i=0; i<WIDGET_COUNT; i++)
    {
        widget = &m_Widgets[i];

        strcpy_s(text, GetButtonText(widget->edge, widget->minimized));
        widget->buttonWidth = m_Font->GetSentencePixelLength(text);

        expandedHeight = WIDGET_HEADER_HEIGHT + (widget->lineCount * WIDGET_LINE_HEIGHT);

        // An expanded widget keeps a margin to its screen edge, a minimized one is only the button and sits flush against the edge.
        edgeInset = widget->minimized ? 0 : WIDGET_MARGIN;

        // Place the panel (the header and the lines under it), then the button on the side of the panel that faces the docked edge.
        switch(widget->edge)
        {
            case DOCK_LEFT:
            {
                panelX = WIDGET_MARGIN;
                panelY = widget->anchor;
                widget->buttonX = edgeInset;
                widget->buttonY = panelY;
                titleX = widget->buttonX + widget->buttonWidth + WIDGET_TITLE_GAP;
                break;
            }

            case DOCK_RIGHT:
            {
                panelX = m_screenWidth - WIDGET_MARGIN - widget->width;
                panelY = widget->anchor;
                widget->buttonX = m_screenWidth - edgeInset - widget->buttonWidth;
                widget->buttonY = panelY;
                titleX = panelX;
                break;
            }

            case DOCK_TOP:
            {
                panelX = widget->anchor;
                panelY = WIDGET_MARGIN;
                widget->buttonX = panelX;
                widget->buttonY = edgeInset;
                titleX = widget->buttonX + widget->buttonWidth + WIDGET_TITLE_GAP;
                break;
            }

            default:
            {
                panelX = widget->anchor;
                panelY = m_screenHeight - WIDGET_MARGIN - expandedHeight;
                widget->buttonX = panelX;
                widget->buttonY = m_screenHeight - edgeInset - WIDGET_HEADER_HEIGHT;
                titleX = widget->buttonX + widget->buttonWidth + WIDGET_TITLE_GAP;
                break;
            }
        }

        widget->lineX = panelX;
        widget->lineY = panelY + WIDGET_HEADER_HEIGHT;

        // Rebuild the button and title text at their new positions. The title only shows while expanded, so it can share the button's row.
        result = widget->button.UpdateText(deviceContext, m_Font, text, widget->buttonX, widget->buttonY, 0.4f, 0.8f, 1.0f);
        if(!result)
        {
            return false;
        }

        strcpy_s(text, widget->titleText);

        result = widget->title.UpdateText(deviceContext, m_Font, text, titleX, widget->buttonY, 1.0f, 1.0f, 1.0f);
        if(!result)
        {
            return false;
        }
    }

    // The mouse and render count strings are rebuilt every frame and the fps string is rebuilt once the previous fps is reset,
    // but the static greeting strings have to be rebuilt here.
    strcpy_s(text, "Hello");
    GetLinePosition(WIDGET_MESSAGES, 0, textX, textY);
    color = m_TextString1->GetPixelColor();

    result = m_TextString1->UpdateText(deviceContext, m_Font, text, textX, textY, color.x, color.y, color.z);
    if(!result)
    {
        return false;
    }

    strcpy_s(text, "Goodbye");
    GetLinePosition(WIDGET_MESSAGES, 1, textX, textY);
    color = m_TextString2->GetPixelColor();

    result = m_TextString2->UpdateText(deviceContext, m_Font, text, textX, textY, color.x, color.y, color.z);
    if(!result)
    {
        return false;
    }

    // Force the fps string to be rebuilt at its new position on the next frame.
    m_previousFps = -1;

    return true;
}

bool Application::HandleWidgetClick(int mouseX, int mouseY)
{
    HudWidget* widget;
    int i, fontHeight;

    if(!m_widgetsInitialized)
    {
        return true;
    }

    fontHeight = m_Font->GetFontHeight();

    for(i=0; i<WIDGET_COUNT; i++)
    {
        widget = &m_Widgets[i];

        // Check if the click landed on this widget's minimize button.
        if((mouseX >= widget->buttonX - WIDGET_BUTTON_PAD) && (mouseX < widget->buttonX + widget->buttonWidth + WIDGET_BUTTON_PAD) &&
           (mouseY >= widget->buttonY) && (mouseY < widget->buttonY + fontHeight))
        {
            // Minimize or restore the widget and place it again.
            widget->minimized = !widget->minimized;

            return LayoutWidgets();
        }
    }

    return true;
}

bool Application::RenderWidgets(XMMATRIX worldMatrix, XMMATRIX baseViewMatrix, XMMATRIX orthoMatrix)
{
    HudWidget* widget;
    int i, j;
    bool result;

    for(i=0; i<WIDGET_COUNT; i++)
    {
        widget = &m_Widgets[i];

        // The button is always shown so the widget can be restored.
        result = RenderText(&widget->button, worldMatrix, baseViewMatrix, orthoMatrix);
        if(!result)
        {
            return false;
        }

        // A minimized widget is only its button.
        if(widget->minimized)
        {
            continue;
        }

        result = RenderText(&widget->title, worldMatrix, baseViewMatrix, orthoMatrix);
        if(!result)
        {
            return false;
        }

        for(j=0; j<widget->lineCount; j++)
        {
            result = RenderText(widget->lines[j], worldMatrix, baseViewMatrix, orthoMatrix);
            if(!result)
            {
                return false;
            }
        }
    }

    return true;
}

bool Application::RenderText(TextClass* text, XMMATRIX worldMatrix, XMMATRIX baseViewMatrix, XMMATRIX orthoMatrix)
{
    // Put the text vertex and index buffers on the pipeline and draw them using the font shader.
    text->Render(m_Direct3D->GetDeviceContext());

    return m_FontShader->Render(m_Direct3D->GetDeviceContext(), text->GetIndexCount(), worldMatrix, baseViewMatrix, orthoMatrix,
                                m_Font->GetTexture(), text->GetPixelColor());
}

void Application::GetLinePosition(int widgetIndex, int lineIndex, int& positionX, int& positionY)
{
    // The lines are stacked below the widget header.
    positionX = m_Widgets[widgetIndex].lineX;
    positionY = m_Widgets[widgetIndex].lineY + (lineIndex * WIDGET_LINE_HEIGHT);
}

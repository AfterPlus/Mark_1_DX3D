#include "SystemClass.h"
#include "resource.h"


SystemClass::SystemClass()
{
    m_Input = 0;
    m_Application = 0;
    m_fullscreen = START_FULLSCREEN;
}


SystemClass::SystemClass(const SystemClass& other)
{
}


SystemClass::~SystemClass()
{
}


bool SystemClass::Initialize()
{
    int screenWidth, screenHeight;
    bool result;


    // Initialize the width and height of the screen to zero before sending the variables into the function.
    screenWidth = 0;
    screenHeight = 0;

    // Initialize the windows api.
    InitializeWindows(screenWidth, screenHeight);

    // Create and initialize the input object. This object will be used to handle reading the keyboard input from the user.
    m_Input = new InputClass;
    m_Input->Initialize();

    // Create and initialize the application class object. This object will handle rendering all the graphics for this application.
    m_Application = new Application;

    result = m_Application->Initialize(screenWidth, screenHeight, m_hwnd);
    if(!result)
    {
        return false;
    }

    return true;
}


void SystemClass::Shutdown()
{
    // Release the application class object.
    if(m_Application)
    {
        m_Application->Shutdown();
        delete m_Application;
        m_Application = 0;
    }

    // Release the input object.
    if(m_Input)
    {
        delete m_Input;
        m_Input = 0;
    }

    // Shutdown the window.
    ShutdownWindows();

    return;
}


void SystemClass::Run()
{
    MSG msg;
    bool done, result;
    
    // Initialize the message structure.
    ZeroMemory(&msg, sizeof(MSG));

    // Loop until there is a quit message from the window or the user.
    done = false;
    while(!done)
    {
        // Handle the windows messages.
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If windows signals to end the application then exit out.
        if(msg.message == WM_QUIT)
        {
            done = true;
        }
        else
        {
            // Otherwise do the frame processing.
            result = Frame();
            if(!result)
            {
                done = true;
            }
        }
    }

    return;
}


bool SystemClass::Frame()
{
    
    bool result;


    // Check if the user pressed escape and wants to exit the application.
    if(m_Input->is_key_down(VK_ESCAPE))
    {
        return false;
    }

    // Do the frame processing for the application class object.
    result = m_Application->Frame();
    if(!result)
    {
        return false;
    }

    return true;
    
}

// TODO : Learn this topic 
LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch(umsg)
    {
        // Check if a key has been pressed on the keyboard.
    case WM_KEYDOWN:
        {
            // F11 toggles windowed/fullscreen. Bit 30 of lparam is set on OS auto-repeat while the
            // key is held, so this only fires once per physical press.
            if(wparam == VK_F11 && !(lparam & 0x40000000))
            {
                ToggleFullscreen();
                return 0;
            }

            // If a key is pressed send it to the input object so it can record that state.
            m_Input->key_down((unsigned int)wparam);
            return 0;
        }

        // Check if a key has been released on the keyboard.
    case WM_KEYUP:
        {
            // If a key is released then send it to the input object so it can unset the state for that key.
            m_Input->key_up((unsigned int)wparam);
            return 0;
        }

        // Any other messages send to the default message handler as our application won't make use of them.
    default:
        {
            return DefWindowProc(hwnd, umsg, wparam, lparam);
        }
    }
}


void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
    WNDCLASSEX wc;
    DWORD style, exStyle;
    int windowX, windowY, windowWidth, windowHeight;

    // Get an external pointer to this object.
    ApplicationHandle = this;

    // Get the instance of this application.
    m_hinstance = GetModuleHandle(NULL);

    // Give the application a name.
    m_applicationName = L"Mark_1_DX3D";

    // Set up the windows class with default settings.
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = m_hinstance;
    wc.hIcon         = (HICON)LoadImage(m_hinstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    wc.hIconSm       = (HICON)LoadImage(m_hinstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = m_applicationName;
    wc.cbSize        = sizeof(WNDCLASSEX);

    // Register the window class.
    RegisterClassEx(&wc);

    // Work out the style, position and size for the starting windowed/fullscreen mode.
    ComputeWindowLayout(m_fullscreen, style, exStyle, windowX, windowY, windowWidth, windowHeight, screenWidth, screenHeight);

    // Create the window with the computed settings and get the handle to it.
    m_hwnd = CreateWindowEx(exStyle, m_applicationName, m_applicationName,
                            style, windowX, windowY, windowWidth, windowHeight, NULL, NULL, m_hinstance, NULL);

    // Bring the window up on the screen and set it as main focus.
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);

    // Show the mouse cursor.
    ShowCursor(true);

    return;
}


void SystemClass::ComputeWindowLayout(bool fullscreen, DWORD& style, DWORD& exStyle, int& x, int& y, int& width, int& height, int& clientWidth, int& clientHeight)
{
    RECT windowRect;

    exStyle = WS_EX_APPWINDOW;

    if(fullscreen)
    {
        // Borderless window filling the primary monitor.
        clientWidth  = GetSystemMetrics(SM_CXSCREEN);
        clientHeight = GetSystemMetrics(SM_CYSCREEN);

        style  = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
        x      = 0;
        y      = 0;
        width  = clientWidth;
        height = clientHeight;
    }
    else
    {
        // Fixed-size window with a title bar, system menu and minimize box, centered on screen.
        clientWidth  = WINDOWED_WIDTH;
        clientHeight = WINDOWED_HEIGHT;

        style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        // Grow the window rect so the client area ends up exactly clientWidth x clientHeight.
        windowRect.left   = 0;
        windowRect.top    = 0;
        windowRect.right  = clientWidth;
        windowRect.bottom = clientHeight;
        AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

        width  = windowRect.right  - windowRect.left;
        height = windowRect.bottom - windowRect.top;
        x = (GetSystemMetrics(SM_CXSCREEN) - width)  / 2;
        y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    }

    return;
}


void SystemClass::ToggleFullscreen()
{
    DWORD style, exStyle;
    int windowX, windowY, windowWidth, windowHeight, clientWidth, clientHeight;

    m_fullscreen = !m_fullscreen;

    ComputeWindowLayout(m_fullscreen, style, exStyle, windowX, windowY, windowWidth, windowHeight, clientWidth, clientHeight);

    SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
    SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(m_hwnd, HWND_TOP, windowX, windowY, windowWidth, windowHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // Resize the swap chain's back buffer, depth buffer and viewport to match the new client area.
    m_Application->OnResize(clientWidth, clientHeight);

    return;
}


void SystemClass::ShutdownWindows()
{
    // Show the mouse cursor.
    ShowCursor(true);

    // Remove the window.
    DestroyWindow(m_hwnd);
    m_hwnd = NULL;

    // Remove the application instance.
    UnregisterClass(m_applicationName, m_hinstance);
    m_hinstance = NULL;

    // Release the pointer to this class.
    ApplicationHandle = NULL;

    return;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    switch(umessage)
    {
        // Check if the window is being destroyed.
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        // Check if the window is being closed.
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }

        // All other messages pass to the message handler in the system class.
        default:
        {
            return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
        }
    }
}

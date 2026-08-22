#include "SystemClass.h"


SystemClass::SystemClass()
{
    m_Application = 0;
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
    return m_Application->Frame();
}

// TODO : Learn this topic 
LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch(umsg)
    {
        // Check if a key has been pressed on the keyboard.
    case WM_KEYDOWN:
        {
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
    int posX, posY;
    
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
    wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm       = wc.hIcon;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = m_applicationName;
    wc.cbSize        = sizeof(WNDCLASSEX);

    // Register the window class.
    RegisterClassEx(&wc);

    // Run windowed at a fixed resolution.
    screenWidth  = 1280;
    screenHeight = 720;

    // Place the window in the middle of the screen.
    posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth)  / 2;
    posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

    // Create the window with the screen settings and get the handle to it.
    m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
                            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
                            posX, posY, screenWidth, screenHeight, NULL, NULL, m_hinstance, NULL);

    // Bring the window up on the screen and set it as main focus.
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);

    // Hide the mouse cursor.
    ShowCursor(true);

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

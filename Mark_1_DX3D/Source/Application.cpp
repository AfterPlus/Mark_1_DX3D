#include "Application.h"
#include "InputClass.h"

Application::Application()
{
    m_Direct3D = 0;
    m_directInput = 0;
    m_mouse = 0;
    m_mousePosition = { 0, 0 };
}


Application::Application(const Application& other)
{
}


Application::~Application()
{
}


bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    HRESULT hresult;
    bool result;

    // Create and initialize new direct X object
    m_Direct3D = new D3dClass;

    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
        return false;
    }

    // Store the screen size and reset the tracked mouse position.
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_mouseX = 0;
    m_mouseY = 0;

    // Initialize the main DirectInput interface.
    hresult = DirectInput8Create((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_directInput, NULL);
    if (FAILED(hresult))
    {
        MessageBox(hwnd, L"Could not initialize DirectInput", L"Error", MB_OK);
        return false;
    }

    // Initialize the DirectInput interface for the mouse.
    hresult = m_directInput->CreateDevice(GUID_SysMouse, &m_mouse, NULL);
    if (FAILED(hresult))
    {
        MessageBox(hwnd, L"Could not create the DirectInput mouse device", L"Error", MB_OK);
        return false;
    }

    // Set the data format for the mouse using the pre-defined mouse data format.
    hresult = m_mouse->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hresult))
    {
        return false;
    }

    // Set the cooperative level of the mouse to share with other programs.
    hresult = m_mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hresult))
    {
        return false;
    }

    // Acquire the mouse.
    m_mouse->Acquire();

    return true;
}


void Application::Shutdown()
{
    // Release the DirectInput mouse device.
    if (m_mouse)
    {
        m_mouse->Unacquire();
        m_mouse->Release();
        m_mouse = nullptr;
    }

    // Release the main DirectInput interface.
    if (m_directInput)
    {
        m_directInput->Release();
        m_directInput = nullptr;
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

    // Update the tracked mouse position for this frame.
    m_mousePosition = MousePosition();

    // Render the graphics scene
    result = Render();
    if (!result)
    {
        return false;
    }
    return true;
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
    // Clear the buffers to begin the scene.
    m_Direct3D->BeginScene(1.0f, 0.5f, 0.5f, 1.0f);


    // Present the rendered scene to the screen.
    m_Direct3D->EndScene();

    return true;
}

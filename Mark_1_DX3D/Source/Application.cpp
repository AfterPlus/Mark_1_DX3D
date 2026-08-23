#include "Application.h"
#include "InputClass.h"

Application::Application()
{
    m_Direct3D = 0;
}


Application::Application(const Application& other)
{
}


Application::~Application()
{
}


bool Application::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    bool result;
    
    // Create and initialize new direct X object
    m_Direct3D = new D3dClass; 
    
    result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
        return false;
    }
    return true;
}


void Application::Shutdown()
{
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
    
    // Render the graphics scene
    result = Render();
    if (!result)
    {
        return false;
    }
    return true;
}

POINT Application::MousePosition(float x, float y)
{
    return POINT();
}


bool Application::Render()
{
    // Clear the buffers to begin the scene.
    m_Direct3D->BeginScene(1.0f, 0.5f, 0.5f, 1.0f);


    // Present the rendered scene to the screen.
    m_Direct3D->EndScene();

    return true;
}

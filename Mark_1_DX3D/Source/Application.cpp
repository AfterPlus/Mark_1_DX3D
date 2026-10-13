#include "Application.h"

Application::Application()
{
    m_Renderer = nullptr;
    m_Hud = nullptr;
    m_Game = nullptr;
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
    bool result;

    // Engine first: renderer (owns Direct3D), then the HUD that draws through it.
    m_Renderer = new RendererClass;

    result = m_Renderer->Initialize(screenWidth, screenHeight, hwnd);
    if(!result)
    {
        return false;   // RendererClass already showed the message box
    }

    m_Hud = new Hud;

    result = m_Hud->Initialize(m_Renderer->GetDirect3D(), screenWidth, screenHeight, hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the HUD.", L"Error", MB_OK);
        return false;
    }

    m_Timer = new TimerClass;

    result = m_Timer->Initialize();
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the timer object.", L"Error", MB_OK);
        return false;
    }

    // Then the game. Swapping the game later is a one-line change right here.
    m_Game = CreateGame();

    result = m_Game->Initialize(m_Renderer, hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the game.", L"Error", MB_OK);
        return false;
    }

    return true;
}

void Application::Shutdown()
{
    // Reverse order of creation: the game and HUD use the renderer's device, so they go first.
    if(m_Game)
    {
        m_Game->Shutdown();
        delete m_Game;
        m_Game = nullptr;
    }

    if(m_Timer)
    {
        delete m_Timer;
        m_Timer = nullptr;
    }

    if(m_Hud)
    {
        m_Hud->Shutdown();
        delete m_Hud;
        m_Hud = nullptr;
    }

    if(m_Renderer)
    {
        m_Renderer->Shutdown();
        delete m_Renderer;
        m_Renderer = nullptr;
    }
}

bool Application::Frame(InputClass* Input)
{
    float dt;
    bool result;

    if(Input->IsEscapePressed())
    {
        return false;
    }

    // Frame time in seconds.
    m_Timer->Frame();
    dt = m_Timer->GetTime();

    // Dragging the window stalls the loop; without a cap the game would jump a huge step afterwards.
    if(dt > 0.1f)
    {
        dt = 0.1f;
    }

    result = m_Hud->Frame(Input);
    if(!result)
    {
        return false;
    }

    result = m_Game->Update(dt, Input);
    if(!result)
    {
        return false;
    }

    // 3D (game), then 2D (hud) on top, then present.
    result = m_Game->Render();
    if(!result)
    {
        return false;
    }

    // Whatever text the game wants on the HUD this frame.
    for(int line=0; line<3; line++)
    {
        char text[32];

        if(m_Game->GetHudLine(line, text, sizeof(text)))
        {
            m_Hud->SetMessage(line, text);
        }
    }

    result = m_Hud->Render();
    if(!result)
    {
        return false;
    }

    m_Renderer->EndFrame();

    return true;
}

bool Application::OnResize(int screenWidth, int screenHeight)
{
    if(!m_Renderer || !m_Hud)
    {
        return false;
    }

    if(!m_Renderer->OnResize(screenWidth, screenHeight))
    {
        return false;
    }

    return m_Hud->OnResize(screenWidth, screenHeight);
}

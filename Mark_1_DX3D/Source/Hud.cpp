#include "Hud.h"

// Moved from Application.cpp lines 3-8.
static const int WIDGET_MARGIN = 10;
static const int WIDGET_LINE_HEIGHT = 30;
static const int WIDGET_HEADER_HEIGHT = 34;
static const int WIDGET_TITLE_GAP = 8;
static const int WIDGET_BUTTON_PAD = 4;

Hud::Hud()
{
    m_Direct3D = nullptr;
    m_screenWidth = 0;
    m_screenHeight = 0;
    m_FontShader = nullptr;
    m_Font = nullptr;
    m_Fps = nullptr;
    m_FpsString = nullptr;
    m_TextString1 = nullptr;
    m_TextString2 = nullptr;
    m_MouseStrings = nullptr;
    m_previousFps = -1;
    m_mouseWasDown = false;
    m_widgetsInitialized = false;
}

Hud::~Hud()
{
}

bool Hud::Initialize(D3dClass* direct3D, int screenWidth, int screenHeight, HWND hwnd)
{
    char fpsString[32];
    char mouseString1[32], mouseString2[32], mouseString3[32];
    char helloString[32], goodbyeString[32];
    CameraClass camera;
    XMMATRIX baseViewMatrix;
    bool result;

    m_Direct3D = direct3D;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Base view matrix for the 2D text (old Application lines 92-97). The Hud makes its own
    // little camera for this, so it no longer depends on the game's camera.
    camera.SetPosition(0.0f, 0.0f, -10.0f);
    camera.Render();
    camera.GetViewMatrix(baseViewMatrix);
    XMStoreFloat4x4(&m_baseViewMatrix, baseViewMatrix);

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

    // Group the text into the minimizable HUD widgets and place them.
    result = InitializeWidgets();
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the HUD widgets.", L"Error", MB_OK);
        return false;
    }


    return true;
}

void Hud::Shutdown()
{
    // Widget buttons and titles.
    ShutdownWidgets();

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

    if(m_MouseStrings)
    {
        m_MouseStrings[0].Shutdown();
        m_MouseStrings[1].Shutdown();
        m_MouseStrings[2].Shutdown();

        delete [] m_MouseStrings;
        m_MouseStrings = nullptr;
    }

    if(m_FpsString)
    {
        m_FpsString->Shutdown();
        delete m_FpsString;
        m_FpsString = nullptr;
    }

    if(m_Fps)
    {
        delete m_Fps;
        m_Fps = nullptr;
    }

    if(m_Font)
    {
        m_Font->Shutdown();
        delete m_Font;
        m_Font = nullptr;
    }

    if(m_FontShader)
    {
        m_FontShader->Shutdown();
        delete m_FontShader;
        m_FontShader = nullptr;
    }

    m_Direct3D = nullptr;   // borrowed, never deleted here
}

bool Hud::Frame(InputClass* input)
{
    int mouseX, mouseY;
    bool mouseDown, result;

    input->GetMouseLocation(mouseX, mouseY);
    mouseDown = input->IsMousePressed();

    // A new left click on a minimize button toggles that widget (press only, not hold).
    if(mouseDown && !m_mouseWasDown && input->IsMouseInWindow())
    {
        result = HandleWidgetClick(mouseX, mouseY);
        if(!result)
        {
            return false;
        }
    }

    m_mouseWasDown = mouseDown;

    result = UpdateMouseStrings(mouseX, mouseY, mouseDown);
    if(!result)
    {
        return false;
    }

    return UpdateFps();
}

bool Hud::Render()
{
    XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;
    bool result;

    m_Direct3D->GetWorldMatrix(worldMatrix);
    m_Direct3D->GetOrthoMatrix(orthoMatrix);
    baseViewMatrix = XMLoadFloat4x4(&m_baseViewMatrix);

    // 2D rendering: no depth test, alpha blending on.
    m_Direct3D->TurnZBufferOff();
    m_Direct3D->EnableAlphaBlending();

    result = RenderWidgets(worldMatrix, baseViewMatrix, orthoMatrix);

    // Always restore the 3D state, even when RenderWidgets failed.
    m_Direct3D->TurnZBufferOn();
    m_Direct3D->DisableAlphaBlending();

    return result;
}

bool Hud::OnResize(int screenWidth, int screenHeight)
{
    int i;

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Every text quad is positioned in pixels relative to the screen centre.
    if(m_FpsString)
    {
        m_FpsString->SetScreenSize(screenWidth, screenHeight);
    }

    if(m_MouseStrings)
    {
        m_MouseStrings[0].SetScreenSize(screenWidth, screenHeight);
        m_MouseStrings[1].SetScreenSize(screenWidth, screenHeight);
        m_MouseStrings[2].SetScreenSize(screenWidth, screenHeight);
    }

    if(m_TextString1)
    {
        m_TextString1->SetScreenSize(screenWidth, screenHeight);
    }

    if(m_TextString2)
    {
        m_TextString2->SetScreenSize(screenWidth, screenHeight);
    }

    for(i=0; i<WIDGET_COUNT; i++)
    {
        m_Widgets[i].title.SetScreenSize(screenWidth, screenHeight);
        m_Widgets[i].button.SetScreenSize(screenWidth, screenHeight);
    }

    // Widgets are docked to screen edges: place them again.
    return LayoutWidgets();
}

bool Hud::UpdateFps()
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
        return true;

    // Store the fps for checking next frame.
    m_previousFps = fps;

    // Truncate the fps to below 100,000.
    if(fps > 99999)
        fps = 99999;

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

bool Hud::UpdateMouseStrings(int mouseX, int mouseY, bool mouseDown)
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

bool Hud::InitializeWidgets()
{
    // The widest line each widget can show, used to size the widget so it does not change width as the numbers change.
    static const char* widestLines[WIDGET_COUNT] = { "Fps: 99999", "Mouse Button: Yes", "Goodbye" };
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

void Hud::ShutdownWidgets()
{
    int i;

    for(i=0; i<WIDGET_COUNT; i++)
    {
        m_Widgets[i].button.Shutdown();
        m_Widgets[i].title.Shutdown();
    }

    m_widgetsInitialized = false;
}

bool Hud::LayoutWidgets()
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

bool Hud::HandleWidgetClick(int mouseX, int mouseY)
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

bool Hud::RenderWidgets(XMMATRIX worldMatrix, XMMATRIX baseViewMatrix, XMMATRIX orthoMatrix)
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

bool Hud::RenderText(TextClass* text, XMMATRIX worldMatrix, XMMATRIX baseViewMatrix, XMMATRIX orthoMatrix)
{
    // Put the text vertex and index buffers on the pipeline and draw them using the font shader.
    text->Render(m_Direct3D->GetDeviceContext());

    return m_FontShader->Render(m_Direct3D->GetDeviceContext(), text->GetIndexCount(), worldMatrix, baseViewMatrix, orthoMatrix,
                                m_Font->GetTexture(), text->GetPixelColor());
}

void Hud::GetLinePosition(int widgetIndex, int lineIndex, int& positionX, int& positionY)
{
    // The lines are stacked below the widget header.
    positionX = m_Widgets[widgetIndex].lineX;
    positionY = m_Widgets[widgetIndex].lineY + (lineIndex * WIDGET_LINE_HEIGHT);
}

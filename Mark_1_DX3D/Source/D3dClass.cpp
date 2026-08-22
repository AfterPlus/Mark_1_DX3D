#include "D3dClass.h"

D3dClass::D3dClass()
{
    m_swapChain = 0;
    m_device = 0;
    m_deviceContext = 0;
    m_renderTargetView = 0;
    m_depthStencilBuffer = 0;
    m_depthStencilState = 0;
    m_depthStencilView = 0;
    m_rasterState = 0;
}

D3dClass::~D3dClass()
{
    
}

D3dClass::D3dClass(const D3dClass&)
{
    
}

bool D3dClass::Initialize(int, int, bool, HWND, bool, float, float)
{
    return true;
}

void D3dClass::Shutdown()
{
    // Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
    if(m_swapChain)
    {
        m_swapChain->SetFullscreenState(false, NULL);
    }

    if(m_rasterState)
    {
        m_rasterState->Release();
        m_rasterState = 0;
    }

    if(m_depthStencilView)
    {
        m_depthStencilView->Release();
        m_depthStencilView = 0;
    }

    if(m_depthStencilState)
    {
        m_depthStencilState->Release();
        m_depthStencilState = 0;
    }

    if(m_depthStencilBuffer)
    {
        m_depthStencilBuffer->Release();
        m_depthStencilBuffer = 0;
    }

    if(m_renderTargetView)
    {
        m_renderTargetView->Release();
        m_renderTargetView = 0;
    }

    if(m_deviceContext)
    {
        m_deviceContext->Release();
        m_deviceContext = 0;
    }

    if(m_device)
    {
        m_device->Release();
        m_device = 0;
    }

    if(m_swapChain)
    {
        m_swapChain->Release();
        m_swapChain = 0;
    }

    return;
}

void D3dClass::BeginScene(float, float, float, float)
{
    float color[4];


    // Setup the color to clear the buffer to.
    //color[0] = red;
    //color[1] = green;
    //color[2] = blue;
    //color[3] = alpha;

    // Clear the back buffer.
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);
    
    // Clear the depth buffer.
    m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    return;
}

void D3dClass::EndScene()
{
    // Present the back buffer to the screen since rendering is complete.
    if(m_vsync_enabled)
    {
        // Lock to screen refresh rate.
        m_swapChain->Present(1, 0);
    }
    else
    {
        // Present as fast as possible.
        m_swapChain->Present(0, 0);
    }

    return;
}

ID3D11Device* D3dClass::GetDevice()
{
    return m_device;
}

ID3D11DeviceContext* D3dClass::GetDeviceContext()
{
    //projectionMatrix = m_projectionMatrix;
    return nullptr;
}

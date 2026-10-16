#include "FoliageShaderClass.h"

FoliageShaderClass::FoliageShaderClass()
{
    m_vertexShader = nullptr;
    m_pixelShader = nullptr;
    m_layout = nullptr;
    m_frameBuffer = nullptr;
    m_modelBuffer = nullptr;
    m_lightBuffer = nullptr;
    m_instanceBuffer = nullptr;
    m_sampleStateClamp = nullptr;
    m_sampleStateWrap = nullptr;
    m_rasterState = nullptr;
    m_blendState = nullptr;
    m_previousRasterState = nullptr;
    m_previousBlendState = nullptr;
    m_previousSampleMask = 0xffffffff;
    m_instanceCapacity = 0;
    m_instanceCount = 0;
    m_modelIndexCount = 0;
}

FoliageShaderClass::~FoliageShaderClass()
{
}

bool FoliageShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
    D3D11_RASTERIZER_DESC rasterDesc;
    D3D11_BLEND_DESC blendDesc;
    wchar_t vsFilename[128], psFilename[128];
    HRESULT result;

    wcscpy_s(vsFilename, 128, L"_Shader/FoliageVS.HLSL");
    wcscpy_s(psFilename, 128, L"_Shader/FoliagePS.HLSL");

    if(!InitializeShader(device, hwnd, vsFilename, psFilename))
    {
        return false;
    }

    // Both sides of every blade.
    ZeroMemory(&rasterDesc, sizeof(rasterDesc));
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.MultisampleEnable = true;
    rasterDesc.AntialiasedLineEnable = false;

    result = device->CreateRasterizerState(&rasterDesc, &m_rasterState);
    if(FAILED(result))
    {
        return false;
    }

    // Alpha-to-coverage: the texture's alpha decides how many MSAA samples a pixel covers. No sorting needed, and
    // the depth buffer stays correct.
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.AlphaToCoverageEnable = true;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    result = device->CreateBlendState(&blendDesc, &m_blendState);
    if(FAILED(result))
    {
        return false;
    }

    return true;
}

void FoliageShaderClass::Shutdown()
{
    if(m_blendState)
    {
        m_blendState->Release();
        m_blendState = nullptr;
    }

    if(m_rasterState)
    {
        m_rasterState->Release();
        m_rasterState = nullptr;
    }

    if(m_instanceBuffer)
    {
        m_instanceBuffer->Release();
        m_instanceBuffer = nullptr;
    }

    m_instanceCapacity = 0;
    m_instanceCount = 0;

    ShutdownShader();
}

bool FoliageShaderClass::SetInstances(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const FoliageInstance* instances, int instanceCount)
{
    D3D11_BUFFER_DESC instanceBufferDesc;
    HRESULT result;

    m_instanceCount = 0;

    if(instanceCount <= 0)
    {
        return true;
    }

    // Grow (with room to spare, so a few more instances next time do not recreate it again). DEFAULT usage, so single
    // ranges can be replaced later with UpdateSubresource while the GPU may still be drawing last frame's data.
    if(instanceCount > m_instanceCapacity)
    {
        if(m_instanceBuffer)
        {
            m_instanceBuffer->Release();
            m_instanceBuffer = nullptr;
        }

        m_instanceCapacity = instanceCount + instanceCount / 2;

        instanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        instanceBufferDesc.ByteWidth = sizeof(FoliageInstance) * m_instanceCapacity;
        instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        instanceBufferDesc.CPUAccessFlags = 0;
        instanceBufferDesc.MiscFlags = 0;
        instanceBufferDesc.StructureByteStride = 0;

        result = device->CreateBuffer(&instanceBufferDesc, NULL, &m_instanceBuffer);
        if(FAILED(result))
        {
            m_instanceCapacity = 0;
            return false;
        }
    }

    m_instanceCount = instanceCount;

    UpdateInstances(deviceContext, instances, 0, instanceCount);

    return true;
}

void FoliageShaderClass::UpdateInstances(ID3D11DeviceContext* deviceContext, const FoliageInstance* instances, int first, int count)
{
    D3D11_BOX box;

    if(!m_instanceBuffer || first < 0 || count <= 0 || first + count > m_instanceCount)
    {
        return;
    }

    box.left = sizeof(FoliageInstance) * first;
    box.right = sizeof(FoliageInstance) * (first + count);
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;

    deviceContext->UpdateSubresource(m_instanceBuffer, 0, &box, instances + first, 0, 0);
}

bool FoliageShaderClass::Begin(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
                               XMFLOAT3 cameraPosition, XMFLOAT3 lightPosition, XMFLOAT4 ambientColor, XMFLOAT4 diffuseColor,
                               const FoliageWind& wind, ID3D11ShaderResourceView* shadowTexture)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    FrameBufferType* frame;
    LightBufferType* light;
    unsigned int stride, offset;
    HRESULT result;

    // Vertex shader constants.
    result = deviceContext->Map(m_frameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result))
    {
        return false;
    }

    frame = (FrameBufferType*)mappedResource.pData;
    frame->view = XMMatrixTranspose(viewMatrix);
    frame->projection = XMMatrixTranspose(projectionMatrix);
    frame->lightPosition = lightPosition;
    frame->time = wind.time;
    frame->cameraPosition = cameraPosition;
    frame->windStrength = wind.strength;
    frame->windDirection = wind.direction;
    frame->windSpeed = wind.speed;
    frame->fadeStart = wind.fadeStart;
    frame->fadeEnd = wind.fadeEnd;
    frame->lodMaxWiden = wind.lodMaxWiden;
    frame->padding = XMFLOAT2(0.0f, 0.0f);

    deviceContext->Unmap(m_frameBuffer, 0);

    deviceContext->VSSetConstantBuffers(0, 1, &m_frameBuffer);
    deviceContext->VSSetConstantBuffers(1, 1, &m_modelBuffer);

    // Pixel shader constants.
    result = deviceContext->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result))
    {
        return false;
    }

    light = (LightBufferType*)mappedResource.pData;
    light->ambientColor = ambientColor;
    light->diffuseColor = diffuseColor;

    deviceContext->Unmap(m_lightBuffer, 0);

    deviceContext->PSSetConstantBuffers(0, 1, &m_lightBuffer);

    // The ground's shadow, looked up at each instance's root.
    deviceContext->PSSetShaderResources(1, 1, &shadowTexture);
    deviceContext->PSSetSamplers(0, 1, &m_sampleStateClamp);

    deviceContext->IASetInputLayout(m_layout);
    deviceContext->VSSetShader(m_vertexShader, NULL, 0);
    deviceContext->PSSetShader(m_pixelShader, NULL, 0);
    deviceContext->PSSetSamplers(1, 1, &m_sampleStateWrap);

    stride = sizeof(FoliageInstance);
    offset = 0;
    deviceContext->IASetVertexBuffers(1, 1, &m_instanceBuffer, &stride, &offset);

    // Remember the scene's states, then switch to two-sided + alpha-to-coverage.
    deviceContext->RSGetState(&m_previousRasterState);
    deviceContext->OMGetBlendState(&m_previousBlendState, m_previousBlendFactor, &m_previousSampleMask);

    deviceContext->RSSetState(m_rasterState);
    deviceContext->OMSetBlendState(m_blendState, NULL, 0xffffffff);

    return true;
}

bool FoliageShaderClass::SetModel(ID3D11DeviceContext* deviceContext, ModelClass* model, float lodDistance, float lodMinKeep)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ModelBufferType* data;
    ID3D11ShaderResourceView* texture;
    HRESULT result;

    result = deviceContext->Map(m_modelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result))
    {
        return false;
    }

    data = (ModelBufferType*)mappedResource.pData;
    data->lodDistance = lodDistance;
    data->lodMinKeep = lodMinKeep;
    data->padding = XMFLOAT2(0.0f, 0.0f);

    deviceContext->Unmap(m_modelBuffer, 0);

    // Slot 0: the model's vertices / indices (ModelClass binds them). Slot 1 (the instances) stays bound from Begin.
    model->Render(deviceContext);
    m_modelIndexCount = model->GetIndexCount();

    texture = model->GetTexture(0);
    deviceContext->PSSetShaderResources(0, 1, &texture);

    return true;
}

void FoliageShaderClass::Draw(ID3D11DeviceContext* deviceContext, int firstInstance, int instanceCount)
{
    if(firstInstance < 0 || firstInstance >= m_instanceCount || instanceCount <= 0)
    {
        return;
    }

    if(firstInstance + instanceCount > m_instanceCount)
    {
        instanceCount = m_instanceCount - firstInstance;
    }

    deviceContext->DrawIndexedInstanced(m_modelIndexCount, instanceCount, 0, 0, firstInstance);
}

void FoliageShaderClass::End(ID3D11DeviceContext* deviceContext)
{
    ID3D11Buffer* noBuffer = nullptr;
    ID3D11ShaderResourceView* noTexture = nullptr;
    unsigned int zero = 0;

    deviceContext->IASetVertexBuffers(1, 1, &noBuffer, &zero, &zero);

    // The shadow texture is a render target again next frame.
    deviceContext->PSSetShaderResources(1, 1, &noTexture);

    deviceContext->RSSetState(m_previousRasterState);
    deviceContext->OMSetBlendState(m_previousBlendState, m_previousBlendFactor, m_previousSampleMask);

    // The Get calls in Begin added a reference.
    if(m_previousRasterState)
    {
        m_previousRasterState->Release();
        m_previousRasterState = nullptr;
    }

    if(m_previousBlendState)
    {
        m_previousBlendState->Release();
        m_previousBlendState = nullptr;
    }
}

bool FoliageShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename)
{
    HRESULT result;
    ID3D10Blob* errorMessage;
    ID3D10Blob* vertexShaderBuffer;
    ID3D10Blob* pixelShaderBuffer;
    D3D11_INPUT_ELEMENT_DESC polygonLayout[6];
    unsigned int numElements, i;
    D3D11_BUFFER_DESC bufferDesc;
    D3D11_SAMPLER_DESC samplerDesc;

    errorMessage = nullptr;
    vertexShaderBuffer = nullptr;
    pixelShaderBuffer = nullptr;

    result = D3DCompileFromFile(vsFilename, NULL, NULL, "FoliageVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
                                &vertexShaderBuffer, &errorMessage);
    if(FAILED(result))
    {
        if(errorMessage)
        {
            OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
        }
        else
        {
            MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
        }

        return false;
    }

    result = D3DCompileFromFile(psFilename, NULL, NULL, "FoliagePixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
                                &pixelShaderBuffer, &errorMessage);
    if(FAILED(result))
    {
        if(errorMessage)
        {
            OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
        }
        else
        {
            MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
        }

        vertexShaderBuffer->Release();
        return false;
    }

    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
    if(FAILED(result))
    {
        return false;
    }

    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
    if(FAILED(result))
    {
        return false;
    }

    // Slot 0: ModelClass vertices (float4 position, uv, normal; the tangent / binormal after them are not read).
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "TEXCOORD";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    polygonLayout[2].SemanticName = "NORMAL";
    polygonLayout[2].SemanticIndex = 0;
    polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[2].InputSlot = 0;
    polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[2].InstanceDataStepRate = 0;

    // Slot 1: one FoliageInstance per copy -> TEXCOORD1 (position, yaw), TEXCOORD2 (height, width, phase, stiffness), TEXCOORD3 (tint, LOD rank).
    for(i=3; i<6; i++)
    {
        polygonLayout[i].SemanticName = "TEXCOORD";
        polygonLayout[i].SemanticIndex = i - 2;
        polygonLayout[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        polygonLayout[i].InputSlot = 1;
        polygonLayout[i].AlignedByteOffset = (i - 3) * 16;
        polygonLayout[i].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
        polygonLayout[i].InstanceDataStepRate = 1;
    }

    numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

    result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_layout);
    if(FAILED(result))
    {
        return false;
    }

    vertexShaderBuffer->Release();
    vertexShaderBuffer = nullptr;

    pixelShaderBuffer->Release();
    pixelShaderBuffer = nullptr;

    // Constant buffers.
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(FrameBufferType);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&bufferDesc, NULL, &m_frameBuffer);
    if(FAILED(result))
    {
        return false;
    }

    bufferDesc.ByteWidth = sizeof(ModelBufferType);

    result = device->CreateBuffer(&bufferDesc, NULL, &m_modelBuffer);
    if(FAILED(result))
    {
        return false;
    }

    bufferDesc.ByteWidth = sizeof(LightBufferType);

    result = device->CreateBuffer(&bufferDesc, NULL, &m_lightBuffer);
    if(FAILED(result))
    {
        return false;
    }

    // Clamp (shadow texture) and wrap (grass texture) samplers.
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = device->CreateSamplerState(&samplerDesc, &m_sampleStateClamp);
    if(FAILED(result))
    {
        return false;
    }

    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    result = device->CreateSamplerState(&samplerDesc, &m_sampleStateWrap);
    if(FAILED(result))
    {
        return false;
    }

    return true;
}

void FoliageShaderClass::ShutdownShader()
{
    if(m_sampleStateWrap)
    {
        m_sampleStateWrap->Release();
        m_sampleStateWrap = nullptr;
    }

    if(m_sampleStateClamp)
    {
        m_sampleStateClamp->Release();
        m_sampleStateClamp = nullptr;
    }

    if(m_lightBuffer)
    {
        m_lightBuffer->Release();
        m_lightBuffer = nullptr;
    }

    if(m_modelBuffer)
    {
        m_modelBuffer->Release();
        m_modelBuffer = nullptr;
    }

    if(m_frameBuffer)
    {
        m_frameBuffer->Release();
        m_frameBuffer = nullptr;
    }

    if(m_layout)
    {
        m_layout->Release();
        m_layout = nullptr;
    }

    if(m_pixelShader)
    {
        m_pixelShader->Release();
        m_pixelShader = nullptr;
    }

    if(m_vertexShader)
    {
        m_vertexShader->Release();
        m_vertexShader = nullptr;
    }
}

void FoliageShaderClass::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
    char* compileErrors;
    unsigned long long bufferSize, i;
    ofstream fout;

    compileErrors = (char*)(errorMessage->GetBufferPointer());
    bufferSize = errorMessage->GetBufferSize();

    fout.open("shader-error.txt");

    for(i=0; i<bufferSize; i++)
    {
        fout << compileErrors[i];
    }

    fout.close();

    errorMessage->Release();
    errorMessage = nullptr;

    MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);
}

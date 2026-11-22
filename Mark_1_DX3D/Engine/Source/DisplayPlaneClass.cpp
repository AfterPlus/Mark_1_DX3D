#include "DisplayPlaneClass.h"

DisplayPlaneClass::DisplayPlaneClass()
{
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_vertexCount = 0;
    m_indexCount = 0;
}

DisplayPlaneClass::DisplayPlaneClass(const DisplayPlaneClass& other)
{
}

DisplayPlaneClass::~DisplayPlaneClass()
{
}

bool DisplayPlaneClass::Initialize(ID3D11Device* device, float width, float height)
{
    return InitializeBuffers(device, width, height);
}

void DisplayPlaneClass::Shutdown()
{
    ShutdownBuffers();

    return;
}

void DisplayPlaneClass::Render(ID3D11DeviceContext* deviceContext)
{
    unsigned int stride, offset;

    stride = sizeof(VertexType);
    offset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return;
}

int DisplayPlaneClass::GetIndexCount()
{
    return m_indexCount;
}

bool DisplayPlaneClass::InitializeBuffers(ID3D11Device* device, float width, float height)
{
    VertexType* vertices;
    unsigned long* indices;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    float left, right, top, bottom;
    HRESULT result;

    m_vertexCount = 6;
    m_indexCount = 6;

    vertices = new VertexType[m_vertexCount];
    indices = new unsigned long[m_indexCount];

    // The plane is centered on its own origin so it can be placed with a translation matrix.
    left = -width / 2.0f;
    right = width / 2.0f;
    top = height / 2.0f;
    bottom = -height / 2.0f;

    // First triangle.
    vertices[0].position = XMFLOAT4(left, top, 0.0f, 1.0f);
    vertices[0].texture = XMFLOAT2(0.0f, 0.0f);

    vertices[1].position = XMFLOAT4(right, bottom, 0.0f, 1.0f);
    vertices[1].texture = XMFLOAT2(1.0f, 1.0f);

    vertices[2].position = XMFLOAT4(left, bottom, 0.0f, 1.0f);
    vertices[2].texture = XMFLOAT2(0.0f, 1.0f);

    // Second triangle.
    vertices[3].position = XMFLOAT4(left, top, 0.0f, 1.0f);
    vertices[3].texture = XMFLOAT2(0.0f, 0.0f);

    vertices[4].position = XMFLOAT4(right, top, 0.0f, 1.0f);
    vertices[4].texture = XMFLOAT2(1.0f, 0.0f);

    vertices[5].position = XMFLOAT4(right, bottom, 0.0f, 1.0f);
    vertices[5].texture = XMFLOAT2(1.0f, 1.0f);

    for(int i=0; i<m_indexCount; i++)
    {
        indices[i] = i;
    }

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if(FAILED(result))
    {
        delete [] vertices;
        delete [] indices;
        return false;
    }

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);

    delete [] vertices;
    delete [] indices;

    return SUCCEEDED(result);
}

void DisplayPlaneClass::ShutdownBuffers()
{
    if(m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    if(m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }

    return;
}
